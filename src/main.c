#include "builtins.h"
#include "executor.h"
#include "jobs.h"
#include "parser.h"
#include "pipeline.h"
#include "shell.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    init_job_table();

    history_path = get_history_file_path();
    if (history_path != NULL) {
        read_history(history_path);
    }

    while (!should_exit) {
        char *line;
        Command *commands;

        line = readline(SHELLX_PROMPT);
        if (line == NULL) {
            printf("\n");
            break;
        }

        if (!is_blank_or_whitespace(line)) {
            if (!is_duplicate_history(line)) {
                add_history(line);
            }

            commands = parse_command_line(line);
            if (commands != NULL) {
                if (commands->next != NULL) {
                    execute_pipeline(commands);
                } else if (is_builtin(commands)) {
                    execute_builtin(commands, &should_exit);
                } else {
                    execute_command(commands);
                }

                free_command_list(commands);
            }
        }

        free(line);
    }

    if (history_path != NULL) {
        write_history(history_path);
        free(history_path);
    }

    clear_history();
    destroy_job_table();

    return 0;
}
