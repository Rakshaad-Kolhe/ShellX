#define _POSIX_C_SOURCE 200809L

#include "alias.h"
#include "expansion.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AliasTable global_alias_table = { .head = NULL, .count = 0 };

void init_alias_table(void)
{
    destroy_alias_table();
    global_alias_table.head = NULL;
    global_alias_table.count = 0;
}

void destroy_alias_table(void)
{
    Alias *current = global_alias_table.head;
    while (current != NULL) {
        Alias *next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }
    global_alias_table.head = NULL;
    global_alias_table.count = 0;
}

int set_alias(const char *name, const char *value)
{
    if (name == NULL || value == NULL || !is_valid_identifier(name)) {
        return 0;
    }

    /* Check if alias already exists and update in-place */
    Alias *current = global_alias_table.head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            char *new_val = strdup(value);
            if (new_val == NULL) {
                return 0;
            }
            free(current->value);
            current->value = new_val;
            return 1;
        }
        current = current->next;
    }

    /* Insert new alias at head */
    Alias *new_alias = malloc(sizeof(Alias));
    if (new_alias == NULL) {
        return 0;
    }

    new_alias->name = strdup(name);
    new_alias->value = strdup(value);
    if (new_alias->name == NULL || new_alias->value == NULL) {
        free(new_alias->name);
        free(new_alias->value);
        free(new_alias);
        return 0;
    }

    new_alias->next = global_alias_table.head;
    global_alias_table.head = new_alias;
    global_alias_table.count++;
    return 1;
}

const char *get_alias(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    Alias *current = global_alias_table.head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

int remove_alias(const char *name)
{
    if (name == NULL) {
        return 0;
    }

    Alias *current = global_alias_table.head;
    Alias *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (prev == NULL) {
                global_alias_table.head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current->name);
            free(current->value);
            free(current);
            global_alias_table.count--;
            return 1;
        }
        prev = current;
        current = current->next;
    }

    return 0;
}

static int compare_alias_ptrs(const void *a, const void *b)
{
    const Alias *alias_a = *(const Alias **)a;
    const Alias *alias_b = *(const Alias **)b;
    return strcmp(alias_a->name, alias_b->name);
}

void print_aliases(void)
{
    if (global_alias_table.count == 0) {
        return;
    }

    Alias **array = malloc(global_alias_table.count * sizeof(Alias *));
    if (array == NULL) {
        /* Fallback: unsorted print */
        Alias *curr = global_alias_table.head;
        while (curr != NULL) {
            printf("alias %s='%s'\n", curr->name, curr->value);
            curr = curr->next;
        }
        fflush(stdout);
        return;
    }

    size_t i = 0;
    Alias *curr = global_alias_table.head;
    while (curr != NULL && i < global_alias_table.count) {
        array[i++] = curr;
        curr = curr->next;
    }

    qsort(array, i, sizeof(Alias *), compare_alias_ptrs);

    for (size_t j = 0; j < i; j++) {
        printf("alias %s='%s'\n", array[j]->name, array[j]->value);
    }
    fflush(stdout);

    free(array);
}

int print_alias(const char *name)
{
    const char *value = get_alias(name);
    if (value == NULL) {
        return 0;
    }
    printf("alias %s='%s'\n", name, value);
    fflush(stdout);
    return 1;
}

/* Dynamic String Buffer for Expansion */
typedef struct DynBuffer {
    char *data;
    size_t length;
    size_t capacity;
} DynBuffer;

static void buf_init(DynBuffer *b)
{
    b->capacity = 64;
    b->length = 0;
    b->data = malloc(b->capacity);
    if (b->data != NULL) {
        b->data[0] = '\0';
    }
}

static int buf_append_str(DynBuffer *b, const char *str)
{
    if (b->data == NULL || str == NULL) return 0;
    size_t slen = strlen(str);
    if (b->length + slen + 1 > b->capacity) {
        size_t ncap = (b->capacity * 2) > (b->length + slen + 1) ? (b->capacity * 2) : (b->length + slen + 64);
        char *ndata = realloc(b->data, ncap);
        if (ndata == NULL) return 0;
        b->data = ndata;
        b->capacity = ncap;
    }
    memcpy(b->data + b->length, str, slen);
    b->length += slen;
    b->data[b->length] = '\0';
    return 1;
}

static int buf_append_char(DynBuffer *b, char c)
{
    char tmp[2] = { c, '\0' };
    return buf_append_str(b, tmp);
}

static void buf_free(DynBuffer *b)
{
    free(b->data);
    b->data = NULL;
    b->length = 0;
    b->capacity = 0;
}

static int is_visited(const char *name, const char *visited[], size_t visited_count)
{
    for (size_t i = 0; i < visited_count; i++) {
        if (strcmp(name, visited[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Expand a single command prefix recursively up to SHELLX_MAX_ALIAS_DEPTH.
 * Returns newly allocated string containing expanded text, or strdup(word) if no alias.
 */
static char *expand_single_command_word(const char *cmd_text, const char *visited[], size_t visited_count, int depth)
{
    if (cmd_text == NULL) {
        return NULL;
    }

    if (depth >= SHELLX_MAX_ALIAS_DEPTH) {
        return strdup(cmd_text);
    }

    /* Skip leading whitespace */
    const char *p = cmd_text;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    if (*p == '\0' || *p == '\'' || *p == '"' || *p == '\\') {
        /* Quoted or escaped command word: not eligible for alias expansion */
        return strdup(cmd_text);
    }

    /* Scan first word */
    const char *word_start = p;
    while (*p != '\0' && !isspace((unsigned char)*p) && *p != '|' && *p != '<' &&
           *p != '>' && *p != '&' && *p != ';' && *p != '\'' && *p != '"' && *p != '\\') {
        p++;
    }

    size_t word_len = (size_t)(p - word_start);
    if (word_len == 0) {
        return strdup(cmd_text);
    }

    char word[256];
    if (word_len >= sizeof(word)) {
        return strdup(cmd_text);
    }
    memcpy(word, word_start, word_len);
    word[word_len] = '\0';

    const char *alias_val = get_alias(word);
    if (alias_val == NULL || is_visited(word, visited, visited_count)) {
        return strdup(cmd_text);
    }

    /* Add to visited array */
    const char *new_visited[SHELLX_MAX_ALIAS_DEPTH + 1];
    for (size_t i = 0; i < visited_count; i++) {
        new_visited[i] = visited[i];
    }
    new_visited[visited_count] = word;

    /* Build intermediate replacement string */
    DynBuffer ib;
    buf_init(&ib);

    /* Append any leading whitespace from original */
    if (word_start > cmd_text) {
        size_t lead_len = (size_t)(word_start - cmd_text);
        char *lead = malloc(lead_len + 1);
        if (lead != NULL) {
            memcpy(lead, cmd_text, lead_len);
            lead[lead_len] = '\0';
            buf_append_str(&ib, lead);
            free(lead);
        }
    }

    buf_append_str(&ib, alias_val);
    buf_append_str(&ib, p); /* Append remainder of command */

    char *intermediate = ib.data;

    /* Recursively check if the replacement itself begins with an alias */
    char *expanded = expand_single_command_word(intermediate, new_visited, visited_count + 1, depth + 1);
    free(intermediate);
    return expanded;
}

char *expand_aliases(const char *input)
{
    if (input == NULL) {
        return NULL;
    }

    if (global_alias_table.count == 0) {
        return strdup(input);
    }

    DynBuffer out;
    buf_init(&out);

    const char *cursor = input;
    const char *segment_start = cursor;
    int in_single_quote = 0;
    int in_double_quote = 0;

    while (*cursor != '\0') {
        if (*cursor == '\\' && !in_single_quote) {
            cursor++;
            if (*cursor != '\0') {
                cursor++;
            }
            continue;
        }

        if (*cursor == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            cursor++;
            continue;
        }

        if (*cursor == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            cursor++;
            continue;
        }

        if (!in_single_quote && !in_double_quote && *cursor == '|') {
            /* Unquoted pipe: segment boundary */
            size_t seg_len = (size_t)(cursor - segment_start);
            char *seg = malloc(seg_len + 1);
            if (seg == NULL) {
                buf_free(&out);
                return strdup(input);
            }
            memcpy(seg, segment_start, seg_len);
            seg[seg_len] = '\0';

            const char *visited[SHELLX_MAX_ALIAS_DEPTH];
            char *expanded_seg = expand_single_command_word(seg, visited, 0, 0);
            free(seg);

            if (expanded_seg != NULL) {
                buf_append_str(&out, expanded_seg);
                free(expanded_seg);
            }

            buf_append_char(&out, '|');
            cursor++;
            segment_start = cursor;
            continue;
        }

        cursor++;
    }

    /* Final segment */
    if (cursor > segment_start) {
        size_t seg_len = (size_t)(cursor - segment_start);
        char *seg = malloc(seg_len + 1);
        if (seg == NULL) {
            buf_free(&out);
            return strdup(input);
        }
        memcpy(seg, segment_start, seg_len);
        seg[seg_len] = '\0';

        const char *visited[SHELLX_MAX_ALIAS_DEPTH];
        char *expanded_seg = expand_single_command_word(seg, visited, 0, 0);
        free(seg);

        if (expanded_seg != NULL) {
            buf_append_str(&out, expanded_seg);
            free(expanded_seg);
        }
    }

    return out.data;
}
