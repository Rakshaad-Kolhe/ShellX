#include "../include/parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void assert_argument(const Command *command, size_t index, const char *expected)
{
    assert(command->args[index] != NULL);
    assert(strcmp(command->args[index], expected) == 0);
}

static void assert_parse_fails(const char *input)
{
    assert(parse_command_line(input) == NULL);
}

static void assert_args_do_not_contain_operators(const Command *commands)
{
    const Command *command = commands;

    while (command != NULL) {
        size_t index;

        for (index = 0; index < command->arg_count; index++) {
            assert(strcmp(command->args[index], "|") != 0);
            assert(strcmp(command->args[index], "<") != 0);
            assert(strcmp(command->args[index], ">") != 0);
            assert(strcmp(command->args[index], ">>") != 0);
            assert(strcmp(command->args[index], "&") != 0);
        }

        command = command->next;
    }
}

static void test_null_input_returns_null(void)
{
    assert(parse_command_line(NULL) == NULL);
}

static void test_empty_string_returns_null(void)
{
    assert(parse_command_line("") == NULL);
}

static void test_spaces_only_returns_null(void)
{
    assert(parse_command_line("      ") == NULL);
}

static void test_mixed_whitespace_only_returns_null(void)
{
    assert(parse_command_line(" \t\n ") == NULL);
}

static void test_one_command(void)
{
    Command *command = parse_command_line("ls");

    assert(command != NULL);
    assert(command->arg_count == 1);
    assert_argument(command, 0, "ls");
    assert(command->args[1] == NULL);
    assert(command->input_path == NULL);
    assert(command->output_path == NULL);
    assert(command->append_output == 0);
    assert(command->run_in_background == 0);
    assert(command->next == NULL);

    free_command_list(command);
}

static void test_command_with_arguments(void)
{
    Command *command = parse_command_line("ls -la /tmp");

    assert(command != NULL);
    assert(command->arg_count == 3);
    assert_argument(command, 0, "ls");
    assert_argument(command, 1, "-la");
    assert_argument(command, 2, "/tmp");
    assert(command->args[3] == NULL);

    free_command_list(command);
}

static void test_leading_and_trailing_whitespace(void)
{
    Command *command = parse_command_line("   grep hello file.txt   \n");

    assert(command != NULL);
    assert(command->arg_count == 3);
    assert_argument(command, 0, "grep");
    assert_argument(command, 1, "hello");
    assert_argument(command, 2, "file.txt");
    assert(command->args[3] == NULL);

    free_command_list(command);
}

static void test_tabs_between_arguments(void)
{
    Command *command = parse_command_line("echo\tone\t\ttwo\n");

    assert(command != NULL);
    assert(command->arg_count == 3);
    assert_argument(command, 0, "echo");
    assert_argument(command, 1, "one");
    assert_argument(command, 2, "two");
    assert(command->args[3] == NULL);

    free_command_list(command);
}

static void test_input_buffer_is_not_modified(void)
{
    char input[] = "cat file.txt";
    Command *command = parse_command_line(input);

    assert(command != NULL);
    assert(strcmp(input, "cat file.txt") == 0);
    assert(command->arg_count == 2);
    assert_argument(command, 0, "cat");
    assert_argument(command, 1, "file.txt");

    free_command_list(command);
}

static void test_arguments_have_independent_storage(void)
{
    Command *command = parse_command_line("echo hello");

    assert(command != NULL);
    assert(command->arg_count == 2);
    assert(command->args[0] != command->args[1]);
    assert_argument(command, 0, "echo");
    assert_argument(command, 1, "hello");

    free_command_list(command);
}

static void test_free_command_list_accepts_null(void)
{
    free_command_list(NULL);
}

static void test_two_command_pipeline(void)
{
    Command *first = parse_command_line("ls | wc");
    Command *second;

    assert(first != NULL);
    assert(first->arg_count == 1);
    assert_argument(first, 0, "ls");
    assert(first->args[1] == NULL);
    assert(first->next != NULL);

    second = first->next;
    assert(second->arg_count == 1);
    assert_argument(second, 0, "wc");
    assert(second->args[1] == NULL);
    assert(second->next == NULL);

    free_command_list(first);
}

static void test_three_command_pipeline(void)
{
    Command *first = parse_command_line("cat file.txt | grep hello | wc -l");
    Command *second;
    Command *third;

    assert(first != NULL);
    assert(first->arg_count == 2);
    assert_argument(first, 0, "cat");
    assert_argument(first, 1, "file.txt");
    assert(first->args[2] == NULL);
    assert(first->next != NULL);

    second = first->next;
    assert(second->arg_count == 2);
    assert_argument(second, 0, "grep");
    assert_argument(second, 1, "hello");
    assert(second->args[2] == NULL);
    assert(second->next != NULL);

    third = second->next;
    assert(third->arg_count == 2);
    assert_argument(third, 0, "wc");
    assert_argument(third, 1, "-l");
    assert(third->args[2] == NULL);
    assert(third->next == NULL);

    free_command_list(first);
}

static void test_pipeline_without_surrounding_spaces(void)
{
    Command *first = parse_command_line("cat file.txt|grep hello");
    Command *second;

    assert(first != NULL);
    assert(first->arg_count == 2);
    assert_argument(first, 0, "cat");
    assert_argument(first, 1, "file.txt");
    assert(first->next != NULL);

    second = first->next;
    assert(second->arg_count == 2);
    assert_argument(second, 0, "grep");
    assert_argument(second, 1, "hello");
    assert(second->next == NULL);

    free_command_list(first);
}

static void test_input_redirection(void)
{
    Command *command = parse_command_line("sort < input.txt");

    assert(command != NULL);
    assert(command->arg_count == 1);
    assert_argument(command, 0, "sort");
    assert(command->args[1] == NULL);
    assert(command->input_path != NULL);
    assert(strcmp(command->input_path, "input.txt") == 0);
    assert(command->output_path == NULL);
    assert(command->append_output == 0);

    free_command_list(command);
}

static void test_output_redirection(void)
{
    Command *command = parse_command_line("echo hello > output.txt");

    assert(command != NULL);
    assert(command->arg_count == 2);
    assert_argument(command, 0, "echo");
    assert_argument(command, 1, "hello");
    assert(command->output_path != NULL);
    assert(strcmp(command->output_path, "output.txt") == 0);
    assert(command->append_output == 0);

    free_command_list(command);
}

static void test_append_output_redirection(void)
{
    Command *command = parse_command_line("echo hello >> output.txt");

    assert(command != NULL);
    assert(command->arg_count == 2);
    assert_argument(command, 0, "echo");
    assert_argument(command, 1, "hello");
    assert(command->output_path != NULL);
    assert(strcmp(command->output_path, "output.txt") == 0);
    assert(command->append_output == 1);

    free_command_list(command);
}

static void test_redirection_without_spaces(void)
{
    Command *command = parse_command_line("sort<input.txt>output.txt");

    assert(command != NULL);
    assert(command->arg_count == 1);
    assert_argument(command, 0, "sort");
    assert(command->input_path != NULL);
    assert(strcmp(command->input_path, "input.txt") == 0);
    assert(command->output_path != NULL);
    assert(strcmp(command->output_path, "output.txt") == 0);
    assert(command->append_output == 0);

    free_command_list(command);
}

static void test_input_and_output_redirection_together(void)
{
    Command *command = parse_command_line("sort < input.txt > output.txt");

    assert(command != NULL);
    assert(command->arg_count == 1);
    assert_argument(command, 0, "sort");
    assert(command->input_path != NULL);
    assert(strcmp(command->input_path, "input.txt") == 0);
    assert(command->output_path != NULL);
    assert(strcmp(command->output_path, "output.txt") == 0);
    assert(command->append_output == 0);

    free_command_list(command);
}

static void test_background_execution(void)
{
    Command *command = parse_command_line("sleep 5 &");

    assert(command != NULL);
    assert(command->arg_count == 2);
    assert_argument(command, 0, "sleep");
    assert_argument(command, 1, "5");
    assert(command->args[2] == NULL);
    assert(command->run_in_background == 1);
    assert(command->next == NULL);

    free_command_list(command);
}

static void test_background_execution_without_spaces(void)
{
    Command *command = parse_command_line("sleep 5&");

    assert(command != NULL);
    assert(command->arg_count == 2);
    assert_argument(command, 0, "sleep");
    assert_argument(command, 1, "5");
    assert(command->args[2] == NULL);
    assert(command->run_in_background == 1);
    assert(command->next == NULL);

    free_command_list(command);
}

static void test_background_pipeline(void)
{
    Command *first = parse_command_line("cat file.txt | grep hello &");
    Command *second;

    assert(first != NULL);
    assert(first->arg_count == 2);
    assert_argument(first, 0, "cat");
    assert_argument(first, 1, "file.txt");
    assert(first->run_in_background == 1);
    assert(first->next != NULL);

    second = first->next;
    assert(second->arg_count == 2);
    assert_argument(second, 0, "grep");
    assert_argument(second, 1, "hello");
    assert(second->run_in_background == 0);
    assert(second->next == NULL);

    free_command_list(first);
}

static void test_operators_are_not_stored_as_arguments(void)
{
    Command *pipeline = parse_command_line("cat file.txt | grep hello");
    Command *redirection = parse_command_line("sort<input.txt>>output.txt");
    Command *background = parse_command_line("sleep 5&");

    assert(pipeline != NULL);
    assert(redirection != NULL);
    assert(background != NULL);

    assert_args_do_not_contain_operators(pipeline);
    assert_args_do_not_contain_operators(redirection);
    assert_args_do_not_contain_operators(background);

    free_command_list(pipeline);
    free_command_list(redirection);
    free_command_list(background);
}

static void test_invalid_pipeline_syntax(void)
{
    assert_parse_fails("|");
    assert_parse_fails("ls |");
    assert_parse_fails("| ls");
    assert_parse_fails("ls || wc");
    assert_parse_fails("cat | | wc");
}

static void test_invalid_redirection_syntax(void)
{
    assert_parse_fails("< input.txt");
    assert_parse_fails("echo >");
    assert_parse_fails("echo >>");
    assert_parse_fails("cat <");
    assert_parse_fails("cat < >");
    assert_parse_fails("cat > >");
}

static void test_invalid_background_syntax(void)
{
    assert_parse_fails("sleep & extra");
    assert_parse_fails("echo hello & wc");
}

static void test_repeated_redirection_is_rejected(void)
{
    assert_parse_fails("cat < one.txt < two.txt");
    assert_parse_fails("echo hi > one.txt > two.txt");
    assert_parse_fails("echo hi >> one.txt >> two.txt");
    assert_parse_fails("echo hi > one.txt >> two.txt");
    assert_parse_fails("echo hi >> one.txt > two.txt");
}

int main(void)
{
    test_null_input_returns_null();
    test_empty_string_returns_null();
    test_spaces_only_returns_null();
    test_mixed_whitespace_only_returns_null();
    test_one_command();
    test_command_with_arguments();
    test_leading_and_trailing_whitespace();
    test_tabs_between_arguments();
    test_input_buffer_is_not_modified();
    test_arguments_have_independent_storage();
    test_free_command_list_accepts_null();
    test_two_command_pipeline();
    test_three_command_pipeline();
    test_pipeline_without_surrounding_spaces();
    test_input_redirection();
    test_output_redirection();
    test_append_output_redirection();
    test_redirection_without_spaces();
    test_input_and_output_redirection_together();
    test_background_execution();
    test_background_execution_without_spaces();
    test_background_pipeline();
    test_operators_are_not_stored_as_arguments();
    test_invalid_pipeline_syntax();
    test_invalid_redirection_syntax();
    test_invalid_background_syntax();
    test_repeated_redirection_is_rejected();

    printf("All parser tests passed.\n");

    return 0;
}
