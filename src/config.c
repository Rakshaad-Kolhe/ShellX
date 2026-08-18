#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "alias.h"
#include "expansion.h"
#include "shell.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *get_prompt(void)
{
    const char *ps1 = getenv("PS1");
    if (ps1 != NULL && ps1[0] != '\0') {
        return ps1;
    }
    return SHELLX_DEFAULT_PROMPT;
}

void initialize_shell_config(void)
{
    init_alias_table();
}

void destroy_shell_config(void)
{
    destroy_alias_table();
}

static const char *skip_spaces(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/*
 * Extract value starting at *cursor.
 * If value starts with quote (' or "), parses until matching closing quote.
 * Otherwise, scans until whitespace or unquoted '#'.
 * Returns newly heap-allocated unquoted value string, or NULL on parse/allocation error.
 */
static char *parse_quoted_or_bare_value(const char **cursor, const char *filename, int line_number)
{
    const char *p = *cursor;

    if (*p == '\'' || *p == '"') {
        char quote = *p++;
        const char *val_start = p;
        while (*p != '\0' && *p != quote) {
            if (*p == '\\' && quote == '"' && (*(p + 1) == '"' || *(p + 1) == '\\')) {
                p += 2;
            } else {
                p++;
            }
        }

        if (*p != quote) {
            fprintf(stderr, "shellx: %s:%d: unterminated quote in value definition\n", filename, line_number);
            return NULL;
        }

        size_t val_len = (size_t)(p - val_start);
        char *val = malloc(val_len + 1);
        if (val == NULL) {
            return NULL;
        }

        /* Copy and handle basic escapes if inside double quote */
        size_t dst = 0;
        for (size_t src = 0; src < val_len; src++) {
            if (quote == '"' && val_start[src] == '\\' && src + 1 < val_len &&
                (val_start[src + 1] == '"' || val_start[src + 1] == '\\')) {
                src++;
                val[dst++] = val_start[src];
            } else {
                val[dst++] = val_start[src];
            }
        }
        val[dst] = '\0';

        *cursor = p + 1; /* consume closing quote */
        return val;
    }

    /* Bare value: ends at whitespace or unquoted '#' */
    const char *val_start = p;
    while (*p != '\0' && !isspace((unsigned char)*p) && *p != '#') {
        p++;
    }

    size_t val_len = (size_t)(p - val_start);
    char *val = malloc(val_len + 1);
    if (val == NULL) {
        return NULL;
    }
    memcpy(val, val_start, val_len);
    val[val_len] = '\0';

    *cursor = p;
    return val;
}

int apply_config_line(const char *line, const char *filename, int line_number)
{
    if (line == NULL) {
        return 0;
    }

    const char *p = skip_spaces(line);

    /* Empty line or comment */
    if (*p == '\0' || *p == '#') {
        return 0;
    }

    /* Optional 'export' prefix */
    if (strncmp(p, "export", 6) == 0 && isspace((unsigned char)p[6])) {
        p = skip_spaces(p + 6);
    }

    /* Check for alias definition */
    if (strncmp(p, "alias", 5) == 0 && (isspace((unsigned char)p[5]) || p[5] == '\0')) {
        p = skip_spaces(p + 5);
        if (*p == '\0' || *p == '#') {
            /* Bare alias in rc file is a no-op */
            return 0;
        }

        /* Parse alias definition: NAME=VALUE or NAME="VALUE" */
        const char *eq = strchr(p, '=');
        if (eq == NULL) {
            fprintf(stderr, "shellx: %s:%d: invalid alias syntax: `%s'\n", filename, line_number, p);
            return 1;
        }

        size_t name_len = (size_t)(eq - p);
        char name[256];
        if (name_len == 0 || name_len >= sizeof(name)) {
            fprintf(stderr, "shellx: %s:%d: invalid alias name\n", filename, line_number);
            return 1;
        }
        memcpy(name, p, name_len);
        name[name_len] = '\0';

        if (!is_valid_identifier(name)) {
            fprintf(stderr, "shellx: %s:%d: `%s': not a valid identifier\n", filename, line_number, name);
            return 1;
        }

        const char *val_cursor = eq + 1;
        char *val = parse_quoted_or_bare_value(&val_cursor, filename, line_number);
        if (val == NULL) {
            return 1;
        }

        if (!set_alias(name, val)) {
            fprintf(stderr, "shellx: %s:%d: failed to set alias `%s'\n", filename, line_number, name);
            free(val);
            return 1;
        }

        free(val);
        return 0;
    }

    /* Check for variable assignment: NAME=VALUE or NAME="VALUE" */
    const char *eq = strchr(p, '=');
    if (eq != NULL) {
        size_t name_len = (size_t)(eq - p);
        char name[256];
        if (name_len == 0 || name_len >= sizeof(name)) {
            fprintf(stderr, "shellx: %s:%d: invalid identifier\n", filename, line_number);
            return 1;
        }
        memcpy(name, p, name_len);
        name[name_len] = '\0';

        if (!is_valid_identifier(name)) {
            fprintf(stderr, "shellx: %s:%d: `%s': not a valid identifier\n", filename, line_number, name);
            return 1;
        }

        const char *val_cursor = eq + 1;
        char *val = parse_quoted_or_bare_value(&val_cursor, filename, line_number);
        if (val == NULL) {
            return 1;
        }

        if (setenv(name, val, 1) != 0) {
            fprintf(stderr, "shellx: %s:%d: setenv failed for `%s'\n", filename, line_number, name);
            free(val);
            return 1;
        }

        free(val);
        return 0;
    }

    fprintf(stderr, "shellx: %s:%d: unrecognized configuration line: `%s'\n", filename, line_number, p);
    return 1;
}

int load_shell_rc(void)
{
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        return 0;
    }

    size_t path_len = strlen(home) + 1 + strlen(SHELLX_RC_FILE) + 1;
    char *path = malloc(path_len);
    if (path == NULL) {
        return 1;
    }
    snprintf(path, path_len, "%s/%s", home, SHELLX_RC_FILE);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        free(path);
        if (errno == ENOENT) {
            /* Missing rc file is harmless */
            return 0;
        }
        perror("shellx: ~/.shellxrc");
        return 1;
    }

    char line_buf[4096];
    int line_number = 1;

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        /* Strip trailing newlines */
        size_t len = strlen(line_buf);
        while (len > 0 && (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r')) {
            line_buf[len - 1] = '\0';
            len--;
        }

        apply_config_line(line_buf, path, line_number);
        line_number++;
    }

    fclose(fp);
    free(path);
    return 0;
}
