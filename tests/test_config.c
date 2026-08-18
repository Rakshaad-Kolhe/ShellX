#define _POSIX_C_SOURCE 200809L

#include "alias.h"
#include "config.h"
#include "expansion.h"
#include "shell.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void test_missing_rc_file(void)
{
    char template_dir[] = "/tmp/shellx_test_missing_rc_XXXXXX";
    char *dir = mkdtemp(template_dir);
    assert(dir != NULL);

    char *old_home = getenv("HOME");
    char *saved_home = old_home ? strdup(old_home) : NULL;

    setenv("HOME", dir, 1);

    init_alias_table();
    assert(load_shell_rc() == 0);

    destroy_alias_table();
    rmdir(dir);

    if (saved_home) {
        setenv("HOME", saved_home, 1);
        free(saved_home);
    } else {
        unsetenv("HOME");
    }
}

static void test_config_environment_assignments(void)
{
    unsetenv("TEST_PROJECT");
    unsetenv("TEST_EDITOR");
    unsetenv("TEST_EMPTY");
    unsetenv("TEST_EXPORTED");

    assert(apply_config_line("TEST_PROJECT=ShellX", "test", 1) == 0);
    assert(getenv("TEST_PROJECT") != NULL && strcmp(getenv("TEST_PROJECT"), "ShellX") == 0);

    assert(apply_config_line("TEST_EDITOR=\"vim -u NONE\"", "test", 2) == 0);
    assert(getenv("TEST_EDITOR") != NULL && strcmp(getenv("TEST_EDITOR"), "vim -u NONE") == 0);

    assert(apply_config_line("TEST_EMPTY=", "test", 3) == 0);
    assert(getenv("TEST_EMPTY") != NULL && strcmp(getenv("TEST_EMPTY"), "") == 0);

    assert(apply_config_line("export TEST_EXPORTED=Active", "test", 4) == 0);
    assert(getenv("TEST_EXPORTED") != NULL && strcmp(getenv("TEST_EXPORTED"), "Active") == 0);

    unsetenv("TEST_PROJECT");
    unsetenv("TEST_EDITOR");
    unsetenv("TEST_EMPTY");
    unsetenv("TEST_EXPORTED");
}

static void test_config_prompt_customization(void)
{
    unsetenv("PS1");
    assert(strcmp(get_prompt(), SHELLX_DEFAULT_PROMPT) == 0);

    assert(apply_config_line("PS1=\"ShellX[custom]$ \"", "test", 1) == 0);
    assert(strcmp(get_prompt(), "ShellX[custom]$ ") == 0);

    unsetenv("PS1");
    assert(strcmp(get_prompt(), SHELLX_DEFAULT_PROMPT) == 0);
}

static void test_alias_management(void)
{
    init_alias_table();

    assert(get_alias("ll") == NULL);
    assert(set_alias("ll", "ls -la") == 1);
    assert(get_alias("ll") != NULL && strcmp(get_alias("ll"), "ls -la") == 0);

    /* Update alias */
    assert(set_alias("ll", "ls -lha") == 1);
    assert(get_alias("ll") != NULL && strcmp(get_alias("ll"), "ls -lha") == 0);

    /* Multiple aliases */
    assert(set_alias("gs", "git status") == 1);
    assert(get_alias("gs") != NULL && strcmp(get_alias("gs"), "git status") == 0);

    /* Removal */
    assert(remove_alias("ll") == 1);
    assert(get_alias("ll") == NULL);
    assert(remove_alias("ll") == 0);

    assert(get_alias("gs") != NULL);
    assert(remove_alias("gs") == 1);
    assert(get_alias("gs") == NULL);

    destroy_alias_table();
}

static void test_config_comments_and_whitespace(void)
{
    assert(apply_config_line("# Top level comment", "test", 1) == 0);
    assert(apply_config_line("   # Indented comment", "test", 2) == 0);
    assert(apply_config_line("      ", "test", 3) == 0);
    assert(apply_config_line("", "test", 4) == 0);
}

static void test_config_quoted_hash_preservation(void)
{
    init_alias_table();

    assert(apply_config_line("alias test_hash=\"echo # not a comment\"", "test", 1) == 0);
    assert(get_alias("test_hash") != NULL && strcmp(get_alias("test_hash"), "echo # not a comment") == 0);

    assert(apply_config_line("alias single_hash='echo # also literal'", "test", 2) == 0);
    assert(get_alias("single_hash") != NULL && strcmp(get_alias("single_hash"), "echo # also literal") == 0);

    destroy_alias_table();
}

static void test_config_malformed_syntax(void)
{
    init_alias_table();

    /* Invalid identifiers */
    assert(apply_config_line("1INVALID=value", "test", 1) != 0);
    assert(apply_config_line("BAD-NAME=value", "test", 2) != 0);

    /* Invalid aliases */
    assert(apply_config_line("alias =invalid", "test", 3) != 0);
    assert(apply_config_line("alias 1BAD=foo", "test", 4) != 0);
    assert(apply_config_line("alias bad_quote=\"unterminated", "test", 5) != 0);
    assert(apply_config_line("random unrecognized config text", "test", 6) != 0);

    destroy_alias_table();
}

static void test_alias_expansion_simple(void)
{
    init_alias_table();
    set_alias("ll", "ls -la");
    set_alias("c", "clear");

    char *exp1 = expand_aliases("ll");
    assert(exp1 != NULL && strcmp(exp1, "ls -la") == 0);
    free(exp1);

    char *exp2 = expand_aliases("ll /tmp /var");
    assert(exp2 != NULL && strcmp(exp2, "ls -la /tmp /var") == 0);
    free(exp2);

    char *exp3 = expand_aliases("echo hello");
    assert(exp3 != NULL && strcmp(exp3, "echo hello") == 0);
    free(exp3);

    destroy_alias_table();
}

static void test_alias_expansion_pipelines(void)
{
    init_alias_table();
    set_alias("ll", "ls -la");
    set_alias("mygrep", "grep -i");

    char *exp = expand_aliases("ll /tmp | mygrep pattern");
    assert(exp != NULL && strcmp(exp, "ls -la /tmp | grep -i pattern") == 0);
    free(exp);

    destroy_alias_table();
}

static void test_alias_expansion_quotes_and_escapes(void)
{
    init_alias_table();
    set_alias("ll", "ls -la");

    /* Escaped command word must NOT expand */
    char *exp1 = expand_aliases("\\ll /tmp");
    assert(exp1 != NULL && strcmp(exp1, "\\ll /tmp") == 0);
    free(exp1);

    /* Single quoted command word must NOT expand */
    char *exp2 = expand_aliases("'ll' /tmp");
    assert(exp2 != NULL && strcmp(exp2, "'ll' /tmp") == 0);
    free(exp2);

    /* Double quoted command word must NOT expand */
    char *exp3 = expand_aliases("\"ll\" /tmp");
    assert(exp3 != NULL && strcmp(exp3, "\"ll\" /tmp") == 0);
    free(exp3);

    destroy_alias_table();
}

static void test_alias_expansion_recursion_protection(void)
{
    init_alias_table();

    /* Self-referential alias */
    set_alias("ls", "ls --color=auto");
    char *exp1 = expand_aliases("ls -la");
    assert(exp1 != NULL && strcmp(exp1, "ls --color=auto -la") == 0);
    free(exp1);

    /* Direct 2-cycle recursion: a -> b -> a */
    set_alias("a", "b");
    set_alias("b", "a");
    char *exp2 = expand_aliases("a");
    assert(exp2 != NULL);
    free(exp2);

    /* Multi-hop chain */
    set_alias("c1", "c2");
    set_alias("c2", "c3");
    set_alias("c3", "echo resolved");
    char *exp3 = expand_aliases("c1 arg1");
    assert(exp3 != NULL && strcmp(exp3, "echo resolved arg1") == 0);
    free(exp3);

    destroy_alias_table();
}

static void test_full_rc_file_loading(void)
{
    char template_dir[] = "/tmp/shellx_test_full_rc_XXXXXX";
    char *dir = mkdtemp(template_dir);
    assert(dir != NULL);

    char rc_path[512];
    snprintf(rc_path, sizeof(rc_path), "%s/%s", dir, SHELLX_RC_FILE);

    FILE *fp = fopen(rc_path, "w");
    assert(fp != NULL);
    fprintf(fp, "# ShellX Startup Configuration\n");
    fprintf(fp, "\n");
    fprintf(fp, "PROJECT=ShellX\n");
    fprintf(fp, "PS1=\"ShellX[test]$ \"\n");
    fprintf(fp, "alias ll=\"ls -la\"\n");
    fprintf(fp, "alias gs=\"git status\"\n");
    fprintf(fp, "# End of file\n");
    fclose(fp);

    char *old_home = getenv("HOME");
    char *saved_home = old_home ? strdup(old_home) : NULL;

    setenv("HOME", dir, 1);
    unsetenv("PROJECT");
    unsetenv("PS1");

    initialize_shell_config();
    assert(load_shell_rc() == 0);

    assert(getenv("PROJECT") != NULL && strcmp(getenv("PROJECT"), "ShellX") == 0);
    assert(strcmp(get_prompt(), "ShellX[test]$ ") == 0);
    assert(get_alias("ll") != NULL && strcmp(get_alias("ll"), "ls -la") == 0);
    assert(get_alias("gs") != NULL && strcmp(get_alias("gs"), "git status") == 0);

    destroy_shell_config();

    unlink(rc_path);
    rmdir(dir);

    if (saved_home) {
        setenv("HOME", saved_home, 1);
        free(saved_home);
    } else {
        unsetenv("HOME");
    }
}

int main(void)
{
    test_missing_rc_file();
    test_config_environment_assignments();
    test_config_prompt_customization();
    test_alias_management();
    test_config_comments_and_whitespace();
    test_config_quoted_hash_preservation();
    test_config_malformed_syntax();
    test_alias_expansion_simple();
    test_alias_expansion_pipelines();
    test_alias_expansion_quotes_and_escapes();
    test_alias_expansion_recursion_protection();
    test_full_rc_file_loading();

    printf("All configuration and alias tests passed successfully.\n");
    return 0;
}
