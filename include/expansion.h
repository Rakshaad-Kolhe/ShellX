#ifndef SHELLX_EXPANSION_H
#define SHELLX_EXPANSION_H

#include <stddef.h>
#include <sys/types.h>

typedef struct ShellContext {
    int last_exit_status;
    pid_t shell_pid;
} ShellContext;

/*
 * Returns nonzero if the given string is a valid shell identifier:
 * [A-Za-z_][A-Za-z0-9_]*
 */
int is_valid_identifier(const char *name);

/*
 * Look up an environment variable by name.
 * If unset, returns an empty string "" (guaranteed non-NULL).
 * The returned string is borrowed and must not be freed.
 */
const char *lookup_variable(const char *name);

/*
 * Format a special parameter ('?' or '$') into the provided buffer.
 * Returns 1 on success, 0 on unsupported parameter character.
 */
int format_special_parameter(char param, const ShellContext *context,
                             char *buffer, size_t buffer_size);

/*
 * Check if the given word prefix is eligible for tilde expansion (~ or ~/...).
 * If eligible, returns 1 and sets *home_dir to the home directory path.
 * Returns 0 if not eligible.
 */
int get_tilde_expansion(const char *word, const char **home_dir);

#endif
