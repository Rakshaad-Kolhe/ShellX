#ifndef SHELLX_ALIAS_H
#define SHELLX_ALIAS_H

#include <stddef.h>

#define SHELLX_MAX_ALIAS_DEPTH 16

typedef struct Alias {
    char *name;
    char *value;
    struct Alias *next;
} Alias;

typedef struct AliasTable {
    Alias *head;
    size_t count;
} AliasTable;

/* Initialize the global alias table */
void init_alias_table(void);

/* Destroy the global alias table and free all allocated nodes */
void destroy_alias_table(void);

/*
 * Set or update an alias.
 * Returns 1 on success, 0 on invalid identifier or allocation failure.
 */
int set_alias(const char *name, const char *value);

/*
 * Look up an alias by name.
 * Returns a borrowed pointer to the alias value, or NULL if not found.
 */
const char *get_alias(const char *name);

/*
 * Remove an alias by name.
 * Returns 1 if removed, 0 if not found.
 */
int remove_alias(const char *name);

/*
 * Print all aliases to stdout in format: alias name='value'
 */
void print_aliases(void);

/*
 * Print a single alias to stdout in format: alias name='value'.
 * Returns 1 if found, 0 if not found.
 */
int print_alias(const char *name);

/*
 * Expand aliases in the given input line before lexical tokenization.
 * Returns a newly heap-allocated string containing the expanded line,
 * which the caller must free(). Returns NULL on allocation error.
 */
char *expand_aliases(const char *input);

#endif
