#define _POSIX_C_SOURCE 200809L

#include "alias.h"
#include "builtins.h"
#include "config.h"
#include "executor.h"
#include "expansion.h"
#include "jobs.h"
#include "parser.h"
#include "pipeline.h"
#include "shell.h"
#include "signals.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

static char *get_history_file_path(void)
{
    const char *home = getenv("HOME");
    char *path;
    size_t path_length;

    if (home == NULL || home[0] == '\0') {
        return NULL;
    }

    path_length = strlen(home) + 1 + strlen(SHELLX_HISTORY_FILE) + 1;
    path = malloc(path_length);
    if (path == NULL) {
        return NULL;
    }

    snprintf(path, path_length, "%s/%s", home, SHELLX_HISTORY_FILE);
    return path;
}

static int is_blank_or_whitespace(const char *str)
{
    if (str == NULL) {
        return 1;
    }

    while (*str != '\0') {
        if (!isspace((unsigned char)*str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

static int is_duplicate_history(const char *line)
{
    HIST_ENTRY *last_entry;

    if (history_length == 0) {
        return 0;
    }

    last_entry = history_get(history_length);
    if (last_entry != NULL && last_entry->line != NULL) {
        return strcmp(last_entry->line, line) == 0;
    }

    return 0;
}

int main(void)
{
    char *history_path;
    int should_exit = 0;
    int last_exit_status = 0;
    pid_t shell_pid = getpid();
    ShellContext context = {
        .last_exit_status = 0,
        .shell_pid = shell_pid
    };
    char cwd[4096];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }

    init_signals();
    init_job_table();
    initialize_shell_config();

    history_path = get_history_file_path();
    if (history_path != NULL) {
        read_history(history_path);
    }

    load_shell_rc();

    while (!should_exit) {
        char *line;
        char *expanded_line;
        Command *commands;

        update_job_status();

        line = readline(get_prompt());
        if (line == NULL) {
            printf("\n");
            break;
        }

        if (!is_blank_or_whitespace(line)) {
            if (!is_duplicate_history(line)) {
                add_history(line);
            }

            expanded_line = expand_aliases(line);
            const char *exec_input = (expanded_line != NULL) ? expanded_line : line;

            context.last_exit_status = last_exit_status;
            commands = parse_command_line_with_context(exec_input, &context);
            if (commands != NULL) {
                if (commands->next != NULL) {
                    int status = execute_pipeline(commands);
                    if (!commands->run_in_background) {
                        last_exit_status = status;
                    }
                } else if (is_builtin(commands)) {
                    int status = execute_builtin(commands, &should_exit);
                    last_exit_status = status;
                } else {
                    int status = execute_command(commands);
                    if (!commands->run_in_background) {
                        last_exit_status = status;
                    }
                }

                free_command_list(commands);
            }

            free(expanded_line);
        }

        free(line);
    }

    if (history_path != NULL) {
        write_history(history_path);
        free(history_path);
    }

    clear_history();
    destroy_job_table();
    destroy_shell_config();

    return 0;
}
