#include "../include/pipeline.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

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
    test_borrowed_command_list_is_not_mutated();

    printf("All pipeline tests passed.\n");

    return 0;
}