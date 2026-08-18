#define _POSIX_C_SOURCE 200809L

#include "expansion.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_valid_identifier(const char *name)
{
    if (name == NULL || *name == '\0') {
        return 0;
    }

    if (!isalpha((unsigned char)*name) && *name != '_') {
        return 0;
    }

    name++;
    while (*name != '\0') {
        if (!isalnum((unsigned char)*name) && *name != '_') {
            return 0;
        }
        name++;
    }

    return 1;
}

const char *lookup_variable(const char *name)
{
    const char *value;

    if (name == NULL || *name == '\0') {
        return "";
    }

    value = getenv(name);
    if (value == NULL) {
        return "";
    }

    return value;
}
int format_special_parameter(char param, const ShellContext *context,
                             char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }

    if (param == '?') {
        int status = (context != NULL) ? context->last_exit_status : 0;
        snprintf(buffer, buffer_size, "%d", status);
        return 1;
    }

    if (param == '$') {
        pid_t pid = (context != NULL && context->shell_pid > 0)
                        ? context->shell_pid
                        : getpid();
        snprintf(buffer, buffer_size, "%d", (int)pid);
        return 1;
    }

    return 0;
}

int get_tilde_expansion(const char *word, const char **home_dir)
{
    const char *home;

    if (word == NULL || home_dir == NULL) {
        return 0;
    }

    if (word[0] == '~' && (word[1] == '/' || word[1] == '\0')) {
        home = getenv("HOME");
        *home_dir = (home != NULL) ? home : "";
        return 1;
    }

    return 0;
}
