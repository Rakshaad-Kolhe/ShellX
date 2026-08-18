#define _POSIX_C_SOURCE 200809L

#include "expansion.h"
#include "lexer.h"
#include "parser.h"
#include "pipeline.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_identifier_validation(void)
{
    assert(is_valid_identifier("a"));
    assert(is_valid_identifier("A"));
    assert(is_valid_identifier("_"));
    assert(is_valid_identifier("_foo123"));
    assert(is_valid_identifier("PATH"));
    assert(is_valid_identifier("SHELL_X_VAR_1"));

    assert(!is_valid_identifier(""));
    assert(!is_valid_identifier("1a"));
    assert(!is_valid_identifier("a-b"));
    assert(!is_valid_identifier("a.b"));
    assert(!is_valid_identifier("a=b"));
    assert(!is_valid_identifier("$a"));
}

static void test_variable_expansion_units(void)
{
    setenv("SHELLX_TEST_VAR", "shellx_val", 1);
    unsetenv("SHELLX_UNSET_VAR");

    assert(strcmp(lookup_variable("SHELLX_TEST_VAR"), "shellx_val") == 0);
    assert(strcmp(lookup_variable("SHELLX_UNSET_VAR"), "") == 0);
}

static void test_special_parameters(void)
{
    ShellContext ctx = { .last_exit_status = 42, .shell_pid = 12345 };
    char buf[64];

    assert(format_special_parameter('?', &ctx, buf, sizeof(buf)));
    assert(strcmp(buf, "42") == 0);

    assert(format_special_parameter('$', &ctx, buf, sizeof(buf)));
    assert(strcmp(buf, "12345") == 0);

    assert(!format_special_parameter('x', &ctx, buf, sizeof(buf)));
}

static void test_tilde_expansion_units(void)
{
    setenv("HOME", "/custom/home", 1);
    const char *home = NULL;

    assert(get_tilde_expansion("~", &home) == 1);
    assert(home != NULL && strcmp(home, "/custom/home") == 0);

    assert(get_tilde_expansion("~/projects/ShellX", &home) == 1);
    assert(home != NULL && strcmp(home, "/custom/home") == 0);

    assert(get_tilde_expansion("~other/path", &home) == 0);
    assert(get_tilde_expansion("foo~bar", &home) == 0);
}

static void test_lexer_single_quotes(void)
{
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };
    TokenList tokens;

    assert(tokenize("echo 'hello $USER | < > &'", &ctx, &tokens) == 1);
    assert(tokens.count == 2);
    assert(tokens.tokens[0].type == TOKEN_WORD && strcmp(tokens.tokens[0].value, "echo") == 0);
    assert(tokens.tokens[1].type == TOKEN_WORD && strcmp(tokens.tokens[1].value, "hello $USER | < > &") == 0);
    free_token_list(&tokens);
}

static void test_lexer_double_quotes(void)
{
    setenv("SHELLX_FOO", "bar", 1);
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };
    TokenList tokens;

    assert(tokenize("echo \"val: $SHELLX_FOO | test\"", &ctx, &tokens) == 1);
    assert(tokens.count == 2);
    assert(tokens.tokens[0].type == TOKEN_WORD && strcmp(tokens.tokens[0].value, "echo") == 0);
    assert(tokens.tokens[1].type == TOKEN_WORD && strcmp(tokens.tokens[1].value, "val: bar | test") == 0);
    free_token_list(&tokens);
}

static void test_lexer_escapes(void)
{
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };
    TokenList tokens;

    assert(tokenize("echo hello\\ world \\| \\& \\\" \\\\", &ctx, &tokens) == 1);
    assert(tokens.count == 6);
    assert(strcmp(tokens.tokens[1].value, "hello world") == 0);
    assert(strcmp(tokens.tokens[2].value, "|") == 0);
    assert(strcmp(tokens.tokens[3].value, "&") == 0);
    assert(strcmp(tokens.tokens[4].value, "\"") == 0);
    assert(strcmp(tokens.tokens[5].value, "\\") == 0);
    free_token_list(&tokens);
}

static void test_lexer_quote_concatenation(void)
{
    setenv("SHELLX_MID", "MIDDLE", 1);
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };
    TokenList tokens;

    assert(tokenize("prefix_'$HOME'_\"$SHELLX_MID\"_suffix", &ctx, &tokens) == 1);
    assert(tokens.count == 1);
    assert(strcmp(tokens.tokens[0].value, "prefix_$HOME_MIDDLE_suffix") == 0);
    free_token_list(&tokens);
}

static void test_lexer_tilde_tokenization(void)
{
    setenv("HOME", "/my/home/dir", 1);
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };
    TokenList tokens;

    assert(tokenize("cd ~", &ctx, &tokens) == 1);
    assert(tokens.count == 2);
    assert(strcmp(tokens.tokens[1].value, "/my/home/dir") == 0);
    free_token_list(&tokens);

    assert(tokenize("cd ~/workspace", &ctx, &tokens) == 1);
    assert(tokens.count == 2);
    assert(strcmp(tokens.tokens[1].value, "/my/home/dir/workspace") == 0);
    free_token_list(&tokens);

    assert(tokenize("echo \"~\"", &ctx, &tokens) == 1);
    assert(tokens.count == 2);
    assert(strcmp(tokens.tokens[1].value, "~") == 0);
    free_token_list(&tokens);
}

static void test_parser_operator_shielding(void)
{
    ShellContext ctx = { .last_exit_status = 7, .shell_pid = 9999 };

    Command *cmd = parse_command_line_with_context("echo 'a | b' | grep a", &ctx);
    assert(cmd != NULL);
    assert(cmd->next != NULL);
    assert(cmd->next->next == NULL);
    assert(cmd->arg_count == 2);
    assert(strcmp(cmd->args[0], "echo") == 0);
    assert(strcmp(cmd->args[1], "a | b") == 0);
    assert(cmd->next->arg_count == 2);
    assert(strcmp(cmd->next->args[0], "grep") == 0);
    assert(strcmp(cmd->next->args[1], "a") == 0);
    free_command_list(cmd);

    cmd = parse_command_line_with_context("echo \"hello > out.txt\" > real_out.txt", &ctx);
    assert(cmd != NULL);
    assert(cmd->next == NULL);
    assert(cmd->arg_count == 2);
    assert(strcmp(cmd->args[0], "echo") == 0);
    assert(strcmp(cmd->args[1], "hello > out.txt") == 0);
    assert(cmd->output_path != NULL && strcmp(cmd->output_path, "real_out.txt") == 0);
    free_command_list(cmd);

    cmd = parse_command_line_with_context("echo \"&\"", &ctx);
    assert(cmd != NULL);
    assert(cmd->run_in_background == 0);
    assert(cmd->arg_count == 2);
    assert(strcmp(cmd->args[1], "&") == 0);
    free_command_list(cmd);

    cmd = parse_command_line_with_context("echo $? $$", &ctx);
    assert(cmd != NULL);
    assert(cmd->arg_count == 3);
    assert(strcmp(cmd->args[1], "7") == 0);
    assert(strcmp(cmd->args[2], "9999") == 0);
    free_command_list(cmd);
}

static void test_malformed_syntax_errors(void)
{
    ShellContext ctx = { .last_exit_status = 0, .shell_pid = getpid() };

    assert(parse_command_line_with_context("echo 'unterminated", &ctx) == NULL);
    assert(parse_command_line_with_context("echo \"unterminated", &ctx) == NULL);
    assert(parse_command_line_with_context("echo trailing\\", &ctx) == NULL);
}

int main(void)
{
    test_identifier_validation();
    test_variable_expansion_units();
    test_special_parameters();
    test_tilde_expansion_units();
    test_lexer_single_quotes();
    test_lexer_double_quotes();
    test_lexer_escapes();
    test_lexer_quote_concatenation();
    test_lexer_tilde_tokenization();
    test_parser_operator_shielding();
    test_malformed_syntax_errors();

    printf("All expansion and quoting tests passed successfully.\n");
    return 0;
}
