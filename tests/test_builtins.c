#define _POSIX_C_SOURCE 200809L

#include "alias.h"
#include "builtins.h"
#include "jobs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_CWD_BUFFER_SIZE 4096

typedef struct SavedEnvironmentValue {
    char *value;
    int was_set;
} SavedEnvironmentValue;

static Command make_command(char **args, size_t arg_count)
{
    Command command;
    memset(&command, 0, sizeof(command));
    command.arg_count = arg_count;
    for (size_t i = 0; i < arg_count; ++i) {
        command.args[i] = args[i];
    }
    return command;
}

static void save_current_working_directory(char *buffer, size_t size)
{
    assert(getcwd(buffer, size) != NULL);
}

static void assert_current_working_directory_is(const char *expected_path)
{
    char current_cwd[TEST_CWD_BUFFER_SIZE];
    save_current_working_directory(current_cwd, sizeof(current_cwd));
    assert(strcmp(current_cwd, expected_path) == 0);
}

static SavedEnvironmentValue save_environment_value(const char *name)
{
    SavedEnvironmentValue saved;
    const char *current_value = getenv(name);
    if (current_value == NULL) {
        saved.value = NULL;
        saved.was_set = 0;
    } else {
        saved.value = strdup(current_value);
        assert(saved.value != NULL);
        saved.was_set = 1;
    }
    return saved;
}

static void restore_environment_value(const char *name, const SavedEnvironmentValue *saved)
{
    if (!saved->was_set) {
        unsetenv(name);
        return;
    }
    assert(setenv(name, saved->value, 1) == 0);
}

static void free_environment_value(SavedEnvironmentValue *saved)
{
    free(saved->value);
    saved->value = NULL;
    saved->was_set = 0;
}

static void test_is_builtin_null_command_returns_zero(void)
{
    assert(is_builtin(NULL) == 0);
}

static void test_is_builtin_empty_command_returns_zero(void)
{
    char *argv[] = {NULL};
    Command command = make_command(argv, 0);
    assert(is_builtin(&command) == 0);
}

static void test_is_builtin_recognizes_cd(void)
{
    char *argv[] = {"cd"};
    Command command = make_command(argv, 1);
    assert(is_builtin(&command) != 0);
}

static void test_is_builtin_recognizes_exit(void)
{
    char *argv[] = {"exit"};
    Command command = make_command(argv, 1);
    assert(is_builtin(&command) != 0);
}

static void test_is_builtin_recognizes_jobs_fg_bg(void)
{
    char *argv_jobs[] = {"jobs"};
    Command cmd_jobs = make_command(argv_jobs, 1);
    assert(is_builtin(&cmd_jobs) != 0);

    char *argv_fg[] = {"fg"};
    Command cmd_fg = make_command(argv_fg, 1);
    assert(is_builtin(&cmd_fg) != 0);

    char *argv_bg[] = {"bg"};
    Command cmd_bg = make_command(argv_bg, 1);
    assert(is_builtin(&cmd_bg) != 0);
}

static void test_is_builtin_recognizes_export_unset_env(void)
{
    char *argv_export[] = {"export"};
    Command cmd_export = make_command(argv_export, 1);
    assert(is_builtin(&cmd_export) != 0);

    char *argv_unset[] = {"unset"};
    Command cmd_unset = make_command(argv_unset, 1);
    assert(is_builtin(&cmd_unset) != 0);

    char *argv_env[] = {"env"};
    Command cmd_env = make_command(argv_env, 1);
    assert(is_builtin(&cmd_env) != 0);
}

static void test_is_builtin_recognizes_alias_unalias(void)
{
    char *argv_alias[] = {"alias"};
    Command cmd_alias = make_command(argv_alias, 1);
    assert(is_builtin(&cmd_alias) != 0);

    char *argv_unalias[] = {"unalias"};
    Command cmd_unalias = make_command(argv_unalias, 1);
    assert(is_builtin(&cmd_unalias) != 0);
}

static void test_is_builtin_rejects_external_command(void)
{
    char *argv[] = {"ls"};
    Command command = make_command(argv, 1);
    assert(is_builtin(&command) == 0);
}

static void test_is_builtin_rejects_similar_names(void)
{
    char *argv1[] = {"cdd"};
    Command cmd1 = make_command(argv1, 1);
    assert(is_builtin(&cmd1) == 0);

    char *argv2[] = {"exitt"};
    Command cmd2 = make_command(argv2, 1);
    assert(is_builtin(&cmd2) == 0);

    char *argv3[] = {"exported"};
    Command cmd3 = make_command(argv3, 1);
    assert(is_builtin(&cmd3) == 0);

    char *argv4[] = {"aliases"};
    Command cmd4 = make_command(argv4, 1);
    assert(is_builtin(&cmd4) == 0);
}

static void test_execute_builtin_null_command_returns_one(void)
{
    int should_exit = 0;
    assert(execute_builtin(NULL, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_empty_command_returns_one(void)
{
    char *argv[] = {NULL};
    Command command = make_command(argv, 0);
    int should_exit = 0;

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_null_should_exit_returns_one(void)
{
    char *argv[] = {"cd"};
    Command command = make_command(argv, 1);
    assert(execute_builtin(&command, NULL) == 1);
}

static void test_execute_builtin_unsupported_command_returns_one_and_leaves_should_exit_zero(void)
{
    char *argv[] = {"ls"};
    Command command = make_command(argv, 1);
    int should_exit = 0;

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_exit_sets_should_exit(void)
{
    char *argv[] = {"exit"};
    Command command = make_command(argv, 1);
    char *original_arg0 = command.args[0];
    size_t original_arg_count = command.arg_count;
    int should_exit = 0;

    assert(execute_builtin(&command, &should_exit) == 0);
    assert(should_exit == 1);
    assert(command.args[0] == original_arg0);
    assert(command.arg_count == original_arg_count);
}

static void test_execute_builtin_exit_with_arguments_returns_one(void)
{
    char *argv[] = {"exit", "42"};
    Command command = make_command(argv, 2);
    int should_exit = 0;

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_cd_to_tmp_changes_and_restores_directory(void)
{
    char *argv[] = {"cd", "/tmp"};
    Command command = make_command(argv, 2);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));

    assert(execute_builtin(&command, &should_exit) == 0);
    assert(should_exit == 0);
    assert_current_working_directory_is("/tmp");

    assert(chdir(original_cwd) == 0);
    assert_current_working_directory_is(original_cwd);
}

static void test_execute_builtin_cd_with_no_argument_uses_home_and_restores_state(void)
{
    char *argv[] = {"cd"};
    Command command = make_command(argv, 1);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    SavedEnvironmentValue saved_home;
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));
    saved_home = save_environment_value("HOME");

    assert(setenv("HOME", "/tmp", 1) == 0);
    assert(execute_builtin(&command, &should_exit) == 0);
    assert(should_exit == 0);
    assert_current_working_directory_is("/tmp");

    assert(chdir(original_cwd) == 0);
    restore_environment_value("HOME", &saved_home);
    free_environment_value(&saved_home);
    assert_current_working_directory_is(original_cwd);
}

static void test_execute_builtin_cd_with_too_many_arguments_fails_without_changing_directory(void)
{
    char *argv[] = {"cd", "/tmp", "/var"};
    Command command = make_command(argv, 3);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
    assert_current_working_directory_is(original_cwd);
}

static void test_execute_builtin_cd_to_nonexistent_path_fails_without_changing_directory(void)
{
    char *argv[] = {"cd", "/shellx_directory_that_should_not_exist_9f27"};
    Command command = make_command(argv, 2);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
    assert_current_working_directory_is(original_cwd);
}

static void test_execute_builtin_cd_without_home_fails_and_does_not_change_directory(void)
{
    char *argv[] = {"cd"};
    Command command = make_command(argv, 1);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    SavedEnvironmentValue saved_home;
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));
    saved_home = save_environment_value("HOME");

    assert(unsetenv("HOME") == 0);
    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
    assert_current_working_directory_is(original_cwd);

    restore_environment_value("HOME", &saved_home);
    free_environment_value(&saved_home);
}

static void test_execute_builtin_cd_updates_pwd(void)
{
    char *argv[] = {"cd", "/tmp"};
    Command cmd = make_command(argv, 2);
    char original_cwd[TEST_CWD_BUFFER_SIZE];
    int should_exit = 0;

    save_current_working_directory(original_cwd, sizeof(original_cwd));

    assert(execute_builtin(&cmd, &should_exit) == 0);
    const char *pwd = getenv("PWD");
    assert(pwd != NULL);
    assert(strcmp(pwd, "/tmp") == 0);

    assert(chdir(original_cwd) == 0);
}

static void test_execute_builtin_export_set_and_overwrite(void)
{
    char *argv1[] = {"export", "SHELLX_EXP_VAR=first_value"};
    Command cmd1 = make_command(argv1, 2);
    int should_exit = 0;

    assert(execute_builtin(&cmd1, &should_exit) == 0);
    assert(should_exit == 0);
    assert(getenv("SHELLX_EXP_VAR") != NULL);
    assert(strcmp(getenv("SHELLX_EXP_VAR"), "first_value") == 0);

    char *argv2[] = {"export", "SHELLX_EXP_VAR=second_value"};
    Command cmd2 = make_command(argv2, 2);
    assert(execute_builtin(&cmd2, &should_exit) == 0);
    assert(getenv("SHELLX_EXP_VAR") != NULL);
    assert(strcmp(getenv("SHELLX_EXP_VAR"), "second_value") == 0);

    unsetenv("SHELLX_EXP_VAR");
}

static void test_execute_builtin_export_invalid_name(void)
{
    char *argv1[] = {"export", "1INVALID=value"};
    Command cmd1 = make_command(argv1, 2);
    int should_exit = 0;

    assert(execute_builtin(&cmd1, &should_exit) == 1);
    assert(should_exit == 0);

    char *argv2[] = {"export", "BAD-NAME=value"};
    Command cmd2 = make_command(argv2, 2);
    assert(execute_builtin(&cmd2, &should_exit) == 1);
}

static void test_execute_builtin_unset(void)
{
    setenv("SHELLX_UNSET_TARGET", "to_be_removed", 1);
    assert(getenv("SHELLX_UNSET_TARGET") != NULL);

    char *argv[] = {"unset", "SHELLX_UNSET_TARGET"};
    Command cmd = make_command(argv, 2);
    int should_exit = 0;

    assert(execute_builtin(&cmd, &should_exit) == 0);
    assert(should_exit == 0);
    assert(getenv("SHELLX_UNSET_TARGET") == NULL);

    /* Unsetting non-existent variable is a no-op that succeeds */
    char *argv2[] = {"unset", "SHELLX_NONEXISTENT_XYZ"};
    Command cmd2 = make_command(argv2, 2);
    assert(execute_builtin(&cmd2, &should_exit) == 0);

    /* Unsetting invalid identifier fails */
    char *argv3[] = {"unset", "1BAD_VAR"};
    Command cmd3 = make_command(argv3, 2);
    assert(execute_builtin(&cmd3, &should_exit) == 1);
}

static void test_execute_builtin_env(void)
{
    char *argv[] = {"env"};
    Command cmd = make_command(argv, 1);
    int should_exit = 0;

    assert(execute_builtin(&cmd, &should_exit) == 0);
    assert(should_exit == 0);

    char *argv_extra[] = {"env", "extra_arg"};
    Command cmd_extra = make_command(argv_extra, 2);
    assert(execute_builtin(&cmd_extra, &should_exit) == 1);
}

static void test_execute_builtin_alias_and_unalias(void)
{
    init_alias_table();
    int should_exit = 0;

    /* Define alias */
    char *argv_set[] = {"alias", "ll=ls -la", "gs=git status"};
    Command cmd_set = make_command(argv_set, 3);
    assert(execute_builtin(&cmd_set, &should_exit) == 0);
    assert(get_alias("ll") != NULL && strcmp(get_alias("ll"), "ls -la") == 0);
    assert(get_alias("gs") != NULL && strcmp(get_alias("gs"), "git status") == 0);

    /* Display single alias */
    char *argv_show[] = {"alias", "ll"};
    Command cmd_show = make_command(argv_show, 2);
    assert(execute_builtin(&cmd_show, &should_exit) == 0);

    /* Display non-existent alias */
    char *argv_missing[] = {"alias", "nonexistent_alias_xyz"};
    Command cmd_missing = make_command(argv_missing, 2);
    assert(execute_builtin(&cmd_missing, &should_exit) == 1);

    /* List all aliases */
    char *argv_list[] = {"alias"};
    Command cmd_list = make_command(argv_list, 1);
    assert(execute_builtin(&cmd_list, &should_exit) == 0);

    /* Unalias */
    char *argv_unalias[] = {"unalias", "ll"};
    Command cmd_unalias = make_command(argv_unalias, 2);
    assert(execute_builtin(&cmd_unalias, &should_exit) == 0);
    assert(get_alias("ll") == NULL);
    assert(get_alias("gs") != NULL);

    /* Unalias missing */
    char *argv_unalias_bad[] = {"unalias", "already_removed"};
    Command cmd_unalias_bad = make_command(argv_unalias_bad, 2);
    assert(execute_builtin(&cmd_unalias_bad, &should_exit) == 1);

    /* Unalias with no args */
    char *argv_unalias_empty[] = {"unalias"};
    Command cmd_unalias_empty = make_command(argv_unalias_empty, 1);
    assert(execute_builtin(&cmd_unalias_empty, &should_exit) == 1);

    destroy_alias_table();
}

int main(void)
{
    test_is_builtin_null_command_returns_zero();
    test_is_builtin_empty_command_returns_zero();
    test_is_builtin_recognizes_cd();
    test_is_builtin_recognizes_exit();
    test_is_builtin_recognizes_jobs_fg_bg();
    test_is_builtin_recognizes_export_unset_env();
    test_is_builtin_recognizes_alias_unalias();
    test_is_builtin_rejects_external_command();
    test_is_builtin_rejects_similar_names();

    test_execute_builtin_null_command_returns_one();
    test_execute_builtin_empty_command_returns_one();
    test_execute_builtin_null_should_exit_returns_one();
    test_execute_builtin_unsupported_command_returns_one_and_leaves_should_exit_zero();
    test_execute_builtin_exit_sets_should_exit();
    test_execute_builtin_exit_with_arguments_returns_one();
    test_execute_builtin_cd_to_tmp_changes_and_restores_directory();
    test_execute_builtin_cd_with_no_argument_uses_home_and_restores_state();
    test_execute_builtin_cd_with_too_many_arguments_fails_without_changing_directory();
    test_execute_builtin_cd_to_nonexistent_path_fails_without_changing_directory();
    test_execute_builtin_cd_without_home_fails_and_does_not_change_directory();
    test_execute_builtin_cd_updates_pwd();

    test_execute_builtin_export_set_and_overwrite();
    test_execute_builtin_export_invalid_name();
    test_execute_builtin_unset();
    test_execute_builtin_env();
    test_execute_builtin_alias_and_unalias();

    printf("All built-in tests passed.\n");

    return 0;
}
