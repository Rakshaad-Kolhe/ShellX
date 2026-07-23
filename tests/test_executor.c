#include "../include/executor.h"

#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static Command make_command(char *const argv[], size_t arg_count)
{
    Command command = {0};
    size_t index;

    command.arg_count = arg_count;

    for (index = 0; index < arg_count; index++) {
        command.args[index] = argv[index];
    }

    command.args[arg_count] = NULL;

    return command;
}

static void make_temp_path(char *buffer, size_t buffer_size, const char *suffix)
{
    int written;

    written = snprintf(buffer, buffer_size, "/tmp/shellx_executor_%ld_%s",
                       (long)getpid(), suffix);
    assert(written > 0);
    assert((size_t)written < buffer_size);
}

static void write_text_file(const char *path, const char *contents)
{
    FILE *file = fopen(path, "wb");

    assert(file != NULL);
    assert(fputs(contents, file) != EOF);
    assert(fclose(file) == 0);
}

static char *read_text_file(const char *path)
{
    FILE *file;
    long file_size;
    char *buffer;
    size_t bytes_read;

    file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    file_size = ftell(file);
    assert(file_size >= 0);
    assert(fseek(file, 0, SEEK_SET) == 0);

    buffer = malloc((size_t)file_size + 1);
    assert(buffer != NULL);

    bytes_read = fread(buffer, 1, (size_t)file_size, file);
    assert(bytes_read == (size_t)file_size);
    buffer[file_size] = '\0';

    assert(fclose(file) == 0);
    return buffer;
}

static void assert_file_contents(const char *path, const char *expected_contents)
{
    char *actual_contents = read_text_file(path);

    assert(strcmp(actual_contents, expected_contents) == 0);
    free(actual_contents);
}

static void remove_file_if_present(const char *path)
{
    unlink(path);
}

static void assert_command_unchanged(const Command *command,
                                     char *const original_args[],
                                     size_t original_arg_count,
                                     const char *original_input_path,
                                     const char *original_output_path,
                                     int original_append_output)
{
    size_t index;

    assert(command->arg_count == original_arg_count);

    for (index = 0; index < original_arg_count; index++) {
        assert(command->args[index] == original_args[index]);
    }

    assert(command->args[original_arg_count] == NULL);
    assert(command->input_path == original_input_path);
    assert(command->output_path == original_output_path);
    assert(command->append_output == original_append_output);
}

static void test_null_command_returns_one(void)
{
    assert(execute_command(NULL) == 1);
}

static void test_empty_command_returns_one(void)
{
    Command command = {0};

    command.arg_count = 0;
    command.args[0] = NULL;

    assert(execute_command(&command) == 1);
}

static void test_true_returns_zero(void)
{
    char *argv[] = {"true"};
    Command command = make_command(argv, 1);

    assert(execute_command(&command) == 0);
}

static void test_false_returns_one(void)
{
    char *argv[] = {"false"};
    Command command = make_command(argv, 1);

    assert(execute_command(&command) == 1);
}

static void test_sh_exit_42_returns_42(void)
{
    char *argv[] = {"sh", "-c", "exit 42"};
    Command command = make_command(argv, 3);

    assert(execute_command(&command) == 42);
}

static void test_arguments_are_passed_to_command(void)
{
    char *argv[] = {"sh", "-c", "test \"$1\" = hello", "shellx", "hello"};
    Command command = make_command(argv, 5);

    assert(execute_command(&command) == 0);
}

static void test_nonexistent_command_returns_127(void)
{
    char *argv[] = {"shellx_command_that_should_not_exist_9f27"};
    Command command = make_command(argv, 1);

    assert(execute_command(&command) == 127);
}

static void test_sigterm_child_returns_128_plus_sigterm(void)
{
    char *argv[] = {"sh", "-c", "kill -TERM $$"};
    Command command = make_command(argv, 3);

    assert(execute_command(&command) == 128 + SIGTERM);
}

static void test_borrowed_command_is_not_mutated(void)
{
    char *argv[] = {"true"};
    Command command = make_command(argv, 1);
    char *original_arg0 = command.args[0];
    size_t original_arg_count = command.arg_count;

    assert(execute_command(&command) == 0);
    assert(command.args[0] == original_arg0);
    assert(command.arg_count == original_arg_count);
}

static void test_output_redirection_overwrites_file(void)
{
    char output_path[256];
    char *argv[] = {"sh", "-c", "printf '%s\n' new-line"};
    Command command = make_command(argv, 3);

    make_temp_path(output_path, sizeof(output_path), "truncate.txt");
    write_text_file(output_path, "old-content\n");

    command.output_path = output_path;
    command.append_output = 0;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "new-line\n");

    remove_file_if_present(output_path);
}

static void test_output_redirection_writes_exact_contents(void)
{
    char output_path[256];
    char *argv[] = {"sh", "-c", "printf '%s\n' hello"};
    Command command = make_command(argv, 3);

    make_temp_path(output_path, sizeof(output_path), "output.txt");

    command.output_path = output_path;
    command.append_output = 0;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "hello\n");

    remove_file_if_present(output_path);
}

static void test_append_redirection_appends_to_existing_file(void)
{
    char output_path[256];
    char *argv[] = {"sh", "-c", "printf '%s\n' second"};
    Command command = make_command(argv, 3);

    make_temp_path(output_path, sizeof(output_path), "append.txt");
    write_text_file(output_path, "first\n");

    command.output_path = output_path;
    command.append_output = 1;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "first\nsecond\n");

    remove_file_if_present(output_path);
}

static void test_input_redirection_copies_file_into_stdout(void)
{
    char input_path[256];
    char output_path[256];
    char *argv[] = {"cat"};
    Command command = make_command(argv, 1);

    make_temp_path(input_path, sizeof(input_path), "input.txt");
    make_temp_path(output_path, sizeof(output_path), "copied.txt");
    write_text_file(input_path, "alpha\nbeta\n");

    command.input_path = input_path;
    command.output_path = output_path;
    command.append_output = 0;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "alpha\nbeta\n");

    remove_file_if_present(input_path);
    remove_file_if_present(output_path);
}

static void test_combined_input_and_output_redirection(void)
{
    char input_path[256];
    char output_path[256];
    char *argv[] = {"sort"};
    Command command = make_command(argv, 1);

    make_temp_path(input_path, sizeof(input_path), "unsorted.txt");
    make_temp_path(output_path, sizeof(output_path), "sorted.txt");
    write_text_file(input_path, "banana\napple\ncarrot\n");

    command.input_path = input_path;
    command.output_path = output_path;
    command.append_output = 0;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "apple\nbanana\ncarrot\n");

    remove_file_if_present(input_path);
    remove_file_if_present(output_path);
}

static void test_nonexistent_input_file_returns_nonzero(void)
{
    char input_path[256];
    char *argv[] = {"cat"};
    Command command = make_command(argv, 1);

    make_temp_path(input_path, sizeof(input_path), "missing-input.txt");
    remove_file_if_present(input_path);

    command.input_path = input_path;

    assert(execute_command(&command) != 0);
}

static void test_borrowed_command_is_not_modified_with_redirection(void)
{
    char output_path[256];
    char *argv[] = {"sh", "-c", "printf '%s\n' borrowed"};
    Command command = make_command(argv, 3);
    char *original_args[] = {command.args[0], command.args[1], command.args[2]};

    make_temp_path(output_path, sizeof(output_path), "borrowed.txt");

    command.output_path = output_path;
    command.append_output = 0;

    size_t original_arg_count = command.arg_count;
    const char *original_input_path = command.input_path;
    const char *original_output_path = command.output_path;
    int original_append_output = command.append_output;

    assert(execute_command(&command) == 0);
    assert_file_contents(output_path, "borrowed\n");
    assert_command_unchanged(&command,
                             original_args,
                             original_arg_count,
                             original_input_path,
                             original_output_path,
                             original_append_output);

    remove_file_if_present(output_path);
}

static void test_background_command_returns_immediately(void)
{
    char *argv[] = {"sleep", "1"};
    Command command = make_command(argv, 2);

    command.run_in_background = 1;

    assert(execute_command(&command) == 0);
}

int main(void)
{
    test_null_command_returns_one();
    test_empty_command_returns_one();
    test_true_returns_zero();
    test_false_returns_one();
    test_sh_exit_42_returns_42();
    test_arguments_are_passed_to_command();
    test_nonexistent_command_returns_127();
    test_sigterm_child_returns_128_plus_sigterm();
    test_borrowed_command_is_not_mutated();
    test_output_redirection_writes_exact_contents();
    test_output_redirection_overwrites_file();
    test_append_redirection_appends_to_existing_file();
    test_input_redirection_copies_file_into_stdout();
    test_combined_input_and_output_redirection();
    test_nonexistent_input_file_returns_nonzero();
    test_borrowed_command_is_not_modified_with_redirection();
    test_background_command_returns_immediately();

    printf("All executor tests passed.\n");

    return 0;
}

