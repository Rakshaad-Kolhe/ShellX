#include "../include/pipeline.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Command make_command(char *const argv[], size_t arg_count)
{
    Command command = {0};
    size_t index;

    assert(arg_count < SHELLX_MAX_ARGS);

    command.arg_count = arg_count;

    for (index = 0; index < arg_count; index++) {
        command.args[index] = argv[index];
    }

    command.args[arg_count] = NULL;
    command.next = NULL;

    return command;
}

static void link_commands(Command *left, Command *right)
{
    left->next = right;
}

static void make_temp_path(char *buffer, size_t buffer_size, const char *suffix)
{
    int written;

    written = snprintf(buffer, buffer_size, "/tmp/shellx_test_pipeline_%ld_%s",
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
                                     const Command *original_next,
                                     char *original_input_path,
                                     char *original_output_path,
                                     int original_append_output,
                                     int original_run_in_background)
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
    assert(command->run_in_background == original_run_in_background);
    assert(command->next == original_next);
}

static void test_null_pipeline_returns_nonzero(void)
{
    assert(execute_pipeline(NULL) != 0);
}

static void test_empty_command_returns_nonzero(void)
{
    Command command = {0};

    command.arg_count = 0;
    command.args[0] = NULL;
    command.next = NULL;

    assert(execute_pipeline(&command) != 0);
}

static void test_invalid_command_inside_linked_pipeline_returns_nonzero(void)
{
    char *first_argv[] = {"true"};
    Command first = make_command(first_argv, 1);
    Command second = {0};

    link_commands(&first, &second);

    assert(execute_pipeline(&first) != 0);
}

static void test_single_successful_command_returns_zero(void)
{
    char *argv[] = {"true"};
    Command command = make_command(argv, 1);

    assert(execute_pipeline(&command) == 0);
}

static void test_single_failing_command_returns_one(void)
{
    char *argv[] = {"false"};
    Command command = make_command(argv, 1);

    assert(execute_pipeline(&command) == 1);
}

static void test_final_exit_status_is_propagated(void)
{
    char *first_argv[] = {"true"};
    char *second_argv[] = {"sh", "-c", "exit 42"};
    Command first = make_command(first_argv, 1);
    Command second = make_command(second_argv, 3);

    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 42);
}

static void test_two_command_pipeline_returns_zero(void)
{
    char *first_argv[] = {"printf", "hello\n"};
    char *second_argv[] = {"grep", "hello"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);

    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 0);
}

static void test_two_command_pipeline_with_no_match_returns_one(void)
{
    char *first_argv[] = {"printf", "hello\n"};
    char *second_argv[] = {"grep", "missing"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);

    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 1);
}

static void test_three_command_pipeline_returns_zero(void)
{
    char *first_argv[] = {"printf", "banana\napple\nbanana\n"};
    char *second_argv[] = {"grep", "banana"};
    char *third_argv[] = {"wc", "-l"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);
    Command third = make_command(third_argv, 2);

    link_commands(&first, &second);
    link_commands(&second, &third);

    assert(execute_pipeline(&first) == 0);
}

static void test_nonexistent_final_command_returns_127(void)
{
    char *first_argv[] = {"true"};
    char *second_argv[] = {"shellx_pipeline_command_that_should_not_exist_9f27"};
    Command first = make_command(first_argv, 1);
    Command second = make_command(second_argv, 1);

    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 127);
}

static void test_final_command_signal_termination_returns_128_plus_sigterm(void)
{
    char *first_argv[] = {"true"};
    char *second_argv[] = {"sh", "-c", "kill -TERM $$"};
    Command first = make_command(first_argv, 1);
    Command second = make_command(second_argv, 3);

    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 128 + SIGTERM);
}

static void test_pipeline_input_redirection(void)
{
    char input_path[256];
    char *first_argv[] = {"cat"};
    char *second_argv[] = {"grep", "target"};
    Command first = make_command(first_argv, 1);
    Command second = make_command(second_argv, 2);

    make_temp_path(input_path, sizeof(input_path), "pipe_in.txt");
    write_text_file(input_path, "ignore\ntarget_data\nother\n");

    first.input_path = input_path;
    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 0);

    remove_file_if_present(input_path);
}

static void test_pipeline_output_redirection(void)
{
    char output_path[256];
    char *first_argv[] = {"printf", "alpha\nbeta\ngamma\n"};
    char *second_argv[] = {"grep", "beta"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);

    make_temp_path(output_path, sizeof(output_path), "pipe_out.txt");

    second.output_path = output_path;
    second.append_output = 0;
    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 0);
    assert_file_contents(output_path, "beta\n");

    remove_file_if_present(output_path);
}

static void test_pipeline_append_redirection(void)
{
    char output_path[256];
    char *first_argv[] = {"printf", "line1\n"};
    char *second_argv[] = {"grep", "line1"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);

    make_temp_path(output_path, sizeof(output_path), "pipe_append.txt");
    write_text_file(output_path, "initial\n");

    second.output_path = output_path;
    second.append_output = 1;
    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 0);
    assert_file_contents(output_path, "initial\nline1\n");

    remove_file_if_present(output_path);
}

static void test_pipeline_mixed_input_and_output_redirection(void)
{
    char input_path[256];
    char output_path[256];
    char *first_argv[] = {"cat"};
    char *second_argv[] = {"grep", "match"};
    char *third_argv[] = {"head", "-n", "1"};
    Command first = make_command(first_argv, 1);
    Command second = make_command(second_argv, 2);
    Command third = make_command(third_argv, 3);

    make_temp_path(input_path, sizeof(input_path), "mixed_in.txt");
    make_temp_path(output_path, sizeof(output_path), "mixed_out.txt");
    write_text_file(input_path, "match_one\nskip\nmatch_two\n");

    first.input_path = input_path;
    third.output_path = output_path;
    third.append_output = 0;

    link_commands(&first, &second);
    link_commands(&second, &third);

    assert(execute_pipeline(&first) == 0);
    assert_file_contents(output_path, "match_one\n");

    remove_file_if_present(input_path);
    remove_file_if_present(output_path);
}

static void test_borrowed_command_list_is_not_mutated(void)
{
    char *first_argv[] = {"printf", "hello\n"};
    char *second_argv[] = {"grep", "hello"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);
    char *first_original_args[] = {first.args[0], first.args[1]};
    char *second_original_args[] = {second.args[0], second.args[1]};
    size_t first_original_arg_count = first.arg_count;
    size_t second_original_arg_count = second.arg_count;
    const Command *first_original_next = first.next;
    const Command *second_original_next = second.next;
    char *first_original_input_path = first.input_path;
    char *first_original_output_path = first.output_path;
    char *second_original_input_path = second.input_path;
    char *second_original_output_path = second.output_path;
    int first_original_append_output = first.append_output;
    int second_original_append_output = second.append_output;
    int first_original_run_in_background = first.run_in_background;
    int second_original_run_in_background = second.run_in_background;

    link_commands(&first, &second);

    first_original_next = first.next;
    second_original_next = second.next;

    assert(execute_pipeline(&first) == 0);

    assert_command_unchanged(&first,
                             first_original_args,
                             first_original_arg_count,
                             first_original_next,
                             first_original_input_path,
                             first_original_output_path,
                             first_original_append_output,
                             first_original_run_in_background);
    assert_command_unchanged(&second,
                             second_original_args,
                             second_original_arg_count,
                             second_original_next,
                             second_original_input_path,
                             second_original_output_path,
                             second_original_append_output,
                             second_original_run_in_background);
}

static void test_background_pipeline_returns_immediately(void)
{
    char *first_argv[] = {"printf", "hello\n"};
    char *second_argv[] = {"grep", "hello"};
    Command first = make_command(first_argv, 2);
    Command second = make_command(second_argv, 2);

    first.run_in_background = 1;
    link_commands(&first, &second);

    assert(execute_pipeline(&first) == 0);
}

int main(void)
{
    test_null_pipeline_returns_nonzero();
    test_empty_command_returns_nonzero();
    test_invalid_command_inside_linked_pipeline_returns_nonzero();
    test_single_successful_command_returns_zero();
    test_single_failing_command_returns_one();
    test_final_exit_status_is_propagated();
    test_two_command_pipeline_returns_zero();
    test_two_command_pipeline_with_no_match_returns_one();
    test_three_command_pipeline_returns_zero();
    test_nonexistent_final_command_returns_127();
    test_final_command_signal_termination_returns_128_plus_sigterm();
    test_pipeline_input_redirection();
    test_pipeline_output_redirection();
    test_pipeline_append_redirection();
    test_pipeline_mixed_input_and_output_redirection();
    test_borrowed_command_list_is_not_mutated();
    test_background_pipeline_returns_immediately();

    printf("All pipeline tests passed.\n");

    return 0;
}