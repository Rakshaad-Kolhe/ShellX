#ifndef SHELLX_LEXER_H
#define SHELLX_LEXER_H

#include "expansion.h"
#include <stddef.h>

typedef enum TokenType {
    TOKEN_WORD,
    TOKEN_PIPE,            /* | */
    TOKEN_REDIRECT_IN,     /* < */
    TOKEN_REDIRECT_OUT,    /* > */
    TOKEN_REDIRECT_APPEND, /* >> */
    TOKEN_BACKGROUND,      /* & */
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct Token {
    TokenType type;
    char *value; /* Dynamically allocated for TOKEN_WORD; NULL for operators */
} Token;

typedef struct TokenList {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;

void init_token_list(TokenList *list);
void free_token_list(TokenList *list);

/*
 * Tokenize input string with quote handling, escaping, and parameter expansion.
 * Returns 1 on success, 0 on lexical/syntax error (e.g. unterminated quotes, lone backslash).
 */
int tokenize(const char *input, const ShellContext *context, TokenList *list);

#endif
