#include "builtins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int execute_cd(const Command *command)
{
    const char *directory;

    if (command->arg_count > 2) {
        fprintf(stderr, "shellx: cd: too many arguments\n");
        return 1;
    }

    if (command->arg_count == 1) {
        directory = getenv("HOME");
        if (directory == NULL || directory[0] == '\0') {
            fprintf(stderr, "shellx: cd: HOME not set\n");
            return 1;
        }
    } else {
        directory = command->args[1];
    }

    if (chdir(directory) != 0) {
        perror("shellx: cd");
        return 1;
    }

    return 0;
}

static int execute_exit(const Command *command, int *should_exit)
{
    if (command->arg_count > 1) {
        fprintf(stderr, "shellx: exit: arguments are not supported\n");
        return 1;
    }

    *should_exit = 1;
    return 0;
}

int is_builtin(const Command *command)
{
    if (command == NULL || command->arg_count == 0 || command->args[0] == NULL) {
        return 0;
    }

    return strcmp(command->args[0], "cd") == 0 ||
           strcmp(command->args[0], "exit") == 0;
}

int execute_builtin(const Command *command, int *should_exit)
{
    if (command == NULL || command->arg_count == 0 ||
        command->args[0] == NULL || should_exit == NULL) {
        return 1;
    }

    *should_exit = 0;

    if (strcmp(command->args[0], "cd") == 0) {
        return execute_cd(command);
    }

    if (strcmp(command->args[0], "exit") == 0) {
        return execute_exit(command, should_exit);
    }

    return 1;
}
