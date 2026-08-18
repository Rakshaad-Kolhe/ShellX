#define _POSIX_C_SOURCE 200809L

#include "../include/builtins.h"

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

static void assert_current_working_directory_is(const char *expected)
{
    char cwd[TEST_CWD_BUFFER_SIZE];

    assert(getcwd(cwd, sizeof(cwd)) != NULL);
    assert(strcmp(cwd, expected) == 0);
}

static void save_current_working_directory(char *buffer, size_t buffer_size)
{
    assert(getcwd(buffer, buffer_size) != NULL);
}

static SavedEnvironmentValue save_environment_value(const char *name)
{
    const char *current_value = getenv(name);
    SavedEnvironmentValue saved;

    saved.value = NULL;
    saved.was_set = current_value != NULL;

    if (current_value != NULL) {
        saved.value = strdup(current_value);
        assert(saved.value != NULL);
    }

    return saved;
}

static void restore_environment_value(const char *name, const SavedEnvironmentValue *saved)
{
    if (saved->was_set) {
        assert(saved->value != NULL);
        assert(setenv(name, saved->value, 1) == 0);
    } else {
        assert(unsetenv(name) == 0);
    }
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
    Command command = {0};

    command.arg_count = 0;
    command.args[0] = NULL;

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
    char *jobs_argv[] = {"jobs"};
    char *fg_argv[] = {"fg"};
    char *bg_argv[] = {"bg"};
    Command jobs_cmd = make_command(jobs_argv, 1);
    Command fg_cmd = make_command(fg_argv, 1);
    Command bg_cmd = make_command(bg_argv, 1);

    assert(is_builtin(&jobs_cmd) != 0);
    assert(is_builtin(&fg_cmd) != 0);
    assert(is_builtin(&bg_cmd) != 0);
}

static void test_is_builtin_recognizes_export_unset_env(void)
{
    char *export_argv[] = {"export"};
    char *unset_argv[] = {"unset"};
    char *env_argv[] = {"env"};
    Command export_cmd = make_command(export_argv, 1);
    Command unset_cmd = make_command(unset_argv, 1);
    Command env_cmd = make_command(env_argv, 1);

    assert(is_builtin(&export_cmd) != 0);
    assert(is_builtin(&unset_cmd) != 0);
    assert(is_builtin(&env_cmd) != 0);
}

static void test_is_builtin_rejects_external_command(void)
{
    char *argv[] = {"ls"};
    Command command = make_command(argv, 1);

    assert(is_builtin(&command) == 0);
}

static void test_is_builtin_rejects_similar_names(void)
{
    char *cdx_argv[] = {"cdx"};
    char *exiting_argv[] = {"exiting"};
    Command cdx_command = make_command(cdx_argv, 1);
    Command exiting_command = make_command(exiting_argv, 1);

    assert(is_builtin(&cdx_command) == 0);
    assert(is_builtin(&exiting_command) == 0);
}

static void test_execute_builtin_null_command_returns_one(void)
{
    int should_exit = 0;

    assert(execute_builtin(NULL, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_empty_command_returns_one(void)
{
    Command command = {0};
    int should_exit = 0;

    command.arg_count = 0;
    command.args[0] = NULL;

    assert(execute_builtin(&command, &should_exit) == 1);
    assert(should_exit == 0);
}

static void test_execute_builtin_null_should_exit_returns_one(void)
{
    char *argv[] = {"exit"};
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

int main(void)
{
    test_is_builtin_null_command_returns_zero();
    test_is_builtin_empty_command_returns_zero();
    test_is_builtin_recognizes_cd();
    test_is_builtin_recognizes_exit();
    test_is_builtin_recognizes_jobs_fg_bg();
    test_is_builtin_recognizes_export_unset_env();
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

    printf("All built-in tests passed.\n");

    return 0;
}
