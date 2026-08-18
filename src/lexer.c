#define _POSIX_C_SOURCE 200809L

#include "lexer.h"
#include "expansion.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StringBuilder {
    char *data;
    size_t length;
    size_t capacity;
} StringBuilder;

static void sb_init(StringBuilder *sb)
{
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
}

static int sb_append_mem(StringBuilder *sb, const char *bytes, size_t len)
{
    if (len == 0) {
        return 1;
    }

    if (sb->length + len + 1 > sb->capacity) {
        size_t new_cap = (sb->capacity == 0) ? 64 : sb->capacity * 2;
        while (sb->length + len + 1 > new_cap) {
            new_cap *= 2;
        }
        char *new_data = realloc(sb->data, new_cap);
        if (new_data == NULL) {
            return 0;
        }
        sb->data = new_data;
        sb->capacity = new_cap;
    }

    memcpy(sb->data + sb->length, bytes, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
    return 1;
}

static int sb_append_char(StringBuilder *sb, char c)
{
    return sb_append_mem(sb, &c, 1);
}

static int sb_append_str(StringBuilder *sb, const char *str)
{
    if (str == NULL) {
        return 1;
    }
    return sb_append_mem(sb, str, strlen(str));
}

static char *sb_take(StringBuilder *sb)
{
    if (sb->data == NULL) {
        char *empty = malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }
    char *result = sb->data;
    sb_init(sb);
    return result;
}

static void sb_free(StringBuilder *sb)
{
    free(sb->data);
    sb_init(sb);
}

void init_token_list(TokenList *list)
{
    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_token_list(TokenList *list)
{
    if (list == NULL) {
        return;
    }

    if (list->tokens != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            free(list->tokens[i].value);
            list->tokens[i].value = NULL;
        }
        free(list->tokens);
        list->tokens = NULL;
    }

    list->count = 0;
    list->capacity = 0;
}

static int append_token(TokenList *list, TokenType type, char *value)
{
    if (list->count + 1 > list->capacity) {
        size_t new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
        Token *new_tokens = realloc(list->tokens, new_cap * sizeof(Token));
        if (new_tokens == NULL) {
            free(value);
            return 0;
        }
        list->tokens = new_tokens;
        list->capacity = new_cap;
    }

    list->tokens[list->count].type = type;
    list->tokens[list->count].value = value;
    list->count++;
    return 1;
}

static void skip_whitespace(const char **cursor)
{
    while (**cursor != '\0' && isspace((unsigned char)(**cursor))) {
        (*cursor)++;
    }
}

static int is_unquoted_operator(char c)
{
    return c == '|' || c == '<' || c == '>' || c == '&';
}

static int scan_and_expand_variable(const char **cursor, const ShellContext *context,
                             StringBuilder *sb)
{
    (*cursor)++; /* skip '$' */

    if (**cursor == '?') {
        char buf[32];
        format_special_parameter('?', context, buf, sizeof(buf));
        (*cursor)++;
        return sb_append_str(sb, buf);
    }

    if (**cursor == '$') {
        char buf[32];
        format_special_parameter('$', context, buf, sizeof(buf));
        (*cursor)++;
        return sb_append_str(sb, buf);
    }

    if (isalpha((unsigned char)(**cursor)) || **cursor == '_') {
        const char *start = *cursor;
        while (isalnum((unsigned char)(**cursor)) || **cursor == '_') {
            (*cursor)++;
        }
        size_t name_len = (size_t)(*cursor - start);
        char var_name[256];
        if (name_len >= sizeof(var_name)) {
            name_len = sizeof(var_name) - 1;
        }
        memcpy(var_name, start, name_len);
        var_name[name_len] = '\0';

        const char *val = lookup_variable(var_name);
        return sb_append_str(sb, val);
    }

    /* Literal '$' */
    return sb_append_char(sb, '$');
}

int tokenize(const char *input, const ShellContext *context, TokenList *list)
{
    const char *cursor;

    if (input == NULL || list == NULL) {
        return 0;
    }

    init_token_list(list);
    cursor = input;

    while (1) {
        skip_whitespace(&cursor);
        if (*cursor == '\0') {
            break;
        }

        /* Check unquoted operators */
        if (*cursor == '|') {
            if (!append_token(list, TOKEN_PIPE, NULL)) {
                free_token_list(list);
                return 0;
            }
            cursor++;
            continue;
        }

        if (*cursor == '<') {
            if (!append_token(list, TOKEN_REDIRECT_IN, NULL)) {
                free_token_list(list);
                return 0;
            }
            cursor++;
            continue;
        }

        if (*cursor == '>') {
            if (*(cursor + 1) == '>') {
                if (!append_token(list, TOKEN_REDIRECT_APPEND, NULL)) {
                    free_token_list(list);
                    return 0;
                }
                cursor += 2;
            } else {
                if (!append_token(list, TOKEN_REDIRECT_OUT, NULL)) {
                    free_token_list(list);
                    return 0;
                }
                cursor++;
            }
            continue;
        }

        if (*cursor == '&') {
            if (!append_token(list, TOKEN_BACKGROUND, NULL)) {
                free_token_list(list);
                return 0;
            }
            cursor++;
            continue;
        }

        /* Scan word */
        StringBuilder sb;
        sb_init(&sb);

        /* Tilde expansion at start of unquoted word */
        const char *home_dir = NULL;
        if (get_tilde_expansion(cursor, &home_dir)) {
            if (!sb_append_str(&sb, home_dir)) {
                sb_free(&sb);
                free_token_list(list);
                return 0;
            }
            cursor++; /* consume '~' */
        }

        while (*cursor != '\0') {
            if (isspace((unsigned char)*cursor) || is_unquoted_operator(*cursor)) {
                break;
            }

            if (*cursor == '\'') {
                /* Single quoted section */
                cursor++; /* skip opening quote */
                while (*cursor != '\0' && *cursor != '\'') {
                    if (!sb_append_char(&sb, *cursor)) {
                        sb_free(&sb);
                        free_token_list(list);
                        return 0;
                    }
                    cursor++;
                }

                if (*cursor != '\'') {
                    /* Unterminated single quote */
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
                cursor++; /* skip closing quote */
            } else if (*cursor == '"') {
                /* Double quoted section */
                cursor++; /* skip opening quote */
                while (*cursor != '\0' && *cursor != '"') {
                    if (*cursor == '\\') {
                        char next = *(cursor + 1);
                        if (next == '$' || next == '"' || next == '\\') {
                            if (!sb_append_char(&sb, next)) {
                                sb_free(&sb);
                                free_token_list(list);
                                return 0;
                            }
                            cursor += 2;
                        } else if (next == '\0') {
                            /* Trailing backslash inside double quote */
                            sb_free(&sb);
                            free_token_list(list);
                            return 0;
                        } else if (!sb_append_char(&sb, '\\')) {
                            sb_free(&sb);
                            free_token_list(list);
                            return 0;
                        } else {
                            cursor++;
                        }
                    } else if (*cursor == '$') {
                        if (!scan_and_expand_variable(&cursor, context, &sb)) {
                            sb_free(&sb);
                            free_token_list(list);
                            return 0;
                        }
                    } else {
                        if (!sb_append_char(&sb, *cursor)) {
                            sb_free(&sb);
                            free_token_list(list);
                            return 0;
                        }
                        cursor++;
                    }
                }

                if (*cursor != '"') {
                    /* Unterminated double quote */
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
                cursor++; /* skip closing quote */
            } else if (*cursor == '\\') {
                /* Backslash escape outside quotes */
                cursor++;
                if (*cursor == '\0') {
                    /* Trailing lone backslash */
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
                if (!sb_append_char(&sb, *cursor)) {
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
                cursor++;
            } else if (*cursor == '$') {
                /* Variable expansion outside quotes */
                if (!scan_and_expand_variable(&cursor, context, &sb)) {
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
            } else {
                /* Ordinary character */
                if (!sb_append_char(&sb, *cursor)) {
                    sb_free(&sb);
                    free_token_list(list);
                    return 0;
                }
                cursor++;
            }
        }

        char *word_value = sb_take(&sb);
        if (word_value == NULL) {
            sb_free(&sb);
            free_token_list(list);
            return 0;
        }

        if (!append_token(list, TOKEN_WORD, word_value)) {
            free(word_value);
            free_token_list(list);
            return 0;
        }
    }

    return 1;
}
