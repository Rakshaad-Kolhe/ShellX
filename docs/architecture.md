# ShellX Technical Architecture

This document provides a comprehensive technical overview of the **ShellX** architecture, detailing the design, data structures, and execution flow of its core subsystems.

---

## Architecture Overview

ShellX is structured into decoupled, modular subsystems:

1. **Lexical Scanner & Quoting Engine (`lexer.c`)**: Tokenizes raw user input with full state tracking for single quotes (`'`), double quotes (`"`), backslash escapes (`\`), and operators (`|`, `<`, `>`, `>>`, `&`).
2. **Expansion Engine (`expansion.c`)**: Evaluates environment variables (`$VAR`), special shell parameters (`$?`, `$$`), and tildes (`~`, `~/...`) within an explicit execution context (`ShellContext`).
3. **Parser & AST Builder (`parser.c`)**: Converts validated token streams into a sanitized linked list of `Command` abstractions.
4. **Built-in Command Engine (`builtins.c`)**: Executes shell state modification commands (`cd`, `exit`, `jobs`, `fg`, `bg`, `export`, `unset`, `env`) directly inside the shell process or within pipeline subshells.
5. **Process Execution Engine (`executor.c`)**: Manages process creation (`fork`), stream redirection (`dup2`), binary execution (`execvp`), and parent synchronization (`waitpid`).
6. **Pipeline Inter-Process Communication (IPC) Subsystem (`pipeline.c`)**: Chains arbitrary numbers of child processes together using an iterative rolling POSIX pipe model.
7. **Job Management & Signal Handling Subsystem (`jobs.c`, `signals.c`)**: Tracks process groups, manages terminal control (`tcsetpgrp`), and provides asynchronous zombie reaping (`SIGCHLD`).

---

## 1. Lexical Scanner & Tokenization (`lexer.c`)

The scanner resides in `include/lexer.h` and `src/lexer.c`.

### Data Structures: `Token` and `TokenList`

```c
typedef enum TokenType {
    TOKEN_WORD,
    TOKEN_PIPE,         /* | */
    TOKEN_REDIRECT_IN,  /* < */
    TOKEN_REDIRECT_OUT, /* > */
    TOKEN_APPEND_OUT,   /* >> */
    TOKEN_AMPERSAND     /* & */
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
} Token;

typedef struct TokenList {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;
```

### Scanning States & Rules
1. **Single-Quote State (`'...'`)**: All characters within single quotes are preserved strictly as literal data. Variable interpolation (`$`), escapes (`\`), and operator recognition are completely disabled.
2. **Double-Quote State (`"..."`)**: Preserves spaces and operators literally while performing variable expansion (`$VAR`, `$?`, `$$`) and respecting escape sequences (`\$`, `\"`, `\\`).
3. **Backslash Escapes (`\`)**:
  - Outside quotes: Quotes the immediate next character literally, stripping the backslash.
  - Inside double quotes: Escapes `$`, `"`, `\`, and `\n`.
4. **Tilde Expansion**: Leading `~` or `~/` at word start resolves to `$HOME` using `get_tilde_expansion()`.
5. **Operator Recognition**: Unquoted `|`, `<`, `>`, `>>`, and `&` emit distinct token types. Quoted or escaped instances emit `TOKEN_WORD` tokens, preventing operator spoofing.

---

## 2. Parameter Expansion & Shell Context (`expansion.c`)

The expansion module resides in `include/expansion.h` and `src/expansion.c`.

### Shell Context Structure

```c
typedef struct ShellContext {
    int last_exit_status;  /* Exit status of previous foreground command */
    pid_t shell_pid;       /* PID of the running ShellX process */
} ShellContext;
```

### Parameter Expansion Logic
- **`$?`**: Formatted as a base-10 decimal string representation of `context->last_exit_status`.
- **`$$`**: Formatted as a base-10 decimal string representation of `context->shell_pid`.
- **`$VAR` / `${VAR}`**: Looked up via `getenv(name)`. Unset variables expand to an empty string `""`.
- **Identifier Validation (`is_valid_identifier`)**: Validates that variable names conform to `[a-zA-Z_][a-zA-Z0-9_]*`.

---

## 3. Parser & Command AST Representation (`parser.c`)

The parser consumes tokens produced by the lexer and builds a singly linked list of `Command` structures:

```c
typedef struct Command {
    char *args[SHELLX_MAX_ARGS];  /* Null-terminated argument array */
    size_t arg_count;             /* Number of valid positional arguments */
    char *input_path;             /* Path for '<' redirection or NULL */
    char *output_path;            /* Path for '>' or '>>' redirection or NULL */
    int append_output;            /* 1 for '>>' append mode, 0 for '>' truncate */
    int run_in_background;        /* 1 if command line terminates with '&' */
    struct Command *next;         /* Pointer to next command in pipeline, or NULL */
} Command;
```

### Memory Ownership Transfer
The parser transfers allocated string ownership from `TokenList` directly into `command->args` and redirection paths by setting token values to `NULL`, eliminating redundant string allocations.

---

## 4. Built-in Environment & State Commands (`builtins.c`)

### Supported Built-ins
- **`export [NAME[=VALUE] ...]`**: Validates identifier syntax and calls `setenv(name, value, 1)`. When invoked without arguments, prints exported variables.
- **`unset [NAME ...]`**: Validates identifier syntax and calls `unsetenv(name)`.
- **`env`**: Dumps all active variables from `environ`. Rejects extraneous positional arguments.
- **`cd [PATH]`**: Updates current directory and automatically synchronizes the `PWD` environment variable via `setenv("PWD", cwd, 1)`.
- **`jobs`, `fg`, `bg`, `exit`**: Manage job table state, foreground terminal handoff, and clean shell shutdown.

---

## 5. Process Execution & Rolling Pipeline IPC

- **Iterative Rolling Pipeline**: Employs an iterative loop keeping at most 2 pipe descriptors active simultaneously in the parent process.
- **Pipeline Subshell Execution**: `spawn_child` detects built-in commands and executes them directly within the child subshell via `execute_builtin`, enabling commands like `env | grep VAR` or `export | sort`.
