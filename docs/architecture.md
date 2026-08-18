# ShellX Subsystem Architecture

This document provides a technical overview of **ShellX**'s core subsystems, data structures, and architectural separation of concerns.

---

## Architectural Subsystem Overview

```
                      +---------------------------------------+
                      |         GNU Readline REPL             |
                      | (Line editing & persistent history)   |
                      +-------------------+-------------------+
                                          |
                                          v
                      +---------------------------------------+
                      |     Lexical Scanner & Expansion       |
                      |   (Quotes, Escapes, $VAR, $?, $$)     |
                      +-------------------+-------------------+
                                          | Token stream
                                          v
                      +---------------------------------------+
                      |           Command Parser              |
                      |  (AST Pipeline linked list builder)   |
                      +-------------------+-------------------+
                                          | Command AST
                                          v
                      +---------------------------------------+
                      |          Pipeline Executor            |
                      |   (Rolling pipe fd IPC orchestrator)  |
                      +---------+-------------------+---------+
                                |                   |
               +----------------+                   +----------------+
               | Child subshell                     | Parent process
               v                                    v
+-------------------------------+   +-------------------------------+
|       Child Process           |   |       Built-in Engine         |
|  - File Redirection (<, >, >>)|   |  - cd (syncs PWD)             |
|  - Binary Execution (execvp)  |   |  - export, unset, env         |
|  - Built-in (pipeline subshell|   |  - jobs, fg, bg, exit         |
+-------------------------------+   +-------------------------------+
                                                    ^
                                                    |
                                    +---------------+---------------+
                                    |   Job Control & Signal Engine |
                                    | - tcsetpgrp() foreground ctrl |
                                    | - SIGCHLD async zombie reaping|
                                    | - SIGINT & SIGTSTP trapping   |
                                    +-------------------------------+
```

---

## 1. Lexical Scanner & Expansion Engine (`lexer.c`, `expansion.c`)

### Responsibilities
- Scans raw input strings into discrete tokens (`TOKEN_WORD`, `TOKEN_PIPE`, `TOKEN_REDIRECT_IN`, `TOKEN_REDIRECT_OUT`, `TOKEN_REDIRECT_APPEND`, `TOKEN_BACKGROUND`).
- Preserves literal characters and disables operator tokenization inside single quotes (`'...'`).
- Handles double-quote state (`"..."`): expands variables (`$VAR`, `$?`, `$$`), handles escape sequences (`\$`, `\"`, `\\`), and shields spaces and operators.
- Implements backslash escaping outside and inside double quotes. Inside double quotes, backslash retains its special escape meaning exclusively when preceding `$`, `"`, or `\\`. All other backslash sequences (e.g. `\n`, `\a`) are preserved literally, adhering to traditional shell quoting conventions.
- Performs tilde expansion (`~`, `~/...`) resolving leading `~` to `$HOME`.
- Validates identifier syntax for environment operations (`[a-zA-Z_][a-zA-Z0-9_]*`).

---

## 2. Command Line Parser (`parser.c`)

### Responsibilities
- Consumes tokens produced by `tokenize()` into a linked list of `Command` structures.
- Populates `args[]`, `input_path`, `output_path`, `append_output`, and `is_background`.
- Enforces syntax validation: flags dangling pipes, missing redirection targets, and invalid syntax with clear error messages.
- Ensures clean memory lifecycle: `free_command_pipeline()` frees all AST nodes and dynamic argument strings.

---

## 3. Pipeline IPC Executor (`pipeline.c`, `executor.c`)

### Responsibilities
- Executes multi-stage pipelines (`cmd1 | cmd2 | cmd3`) using a rolling pipe file descriptor pair model.
- Prevents file descriptor leaks by closing unused pipe ends in both parent and child processes.
- Isolates child pipelines into dedicated process groups (`setpgid`).
- Supports subshell execution of built-in commands when chained in pipelines (`spawn_child`).

---

## 4. Built-in Command Subsystem (`builtins.c`)

### Built-in Catalog
- **`cd [DIR]`**: Changes working directory and dynamically updates `PWD` via `setenv("PWD", cwd, 1)`.
- **`export [NAME[=VALUE] ...]`**: Sets environment variables or lists exported variables with `is_valid_identifier` validation.
- **`unset [NAME ...]`**: Removes environment variables via `unsetenv`.
- **`env`**: Dumps all active environment entries from `environ` (rejects positional arguments).
- **`jobs`**: Displays active, stopped, and background jobs.
- **`fg [JOB_ID]`**: Brings a background or stopped job to the foreground and hands over terminal ownership via `tcsetpgrp()`.
- **`bg [JOB_ID]`**: Resumes a stopped job in the background via `kill(-pgid, SIGCONT)`.
- **`exit`**: Gracefully terminates the shell session.

---

## 5. POSIX Job Control & Signal Management (`jobs.c`, `signals.c`)

### Responsibilities
- Tracks process groups and lifecycle states (`JOB_RUNNING`, `JOB_STOPPED`, `JOB_COMPLETED`).
- Non-blocking `SIGCHLD` handler uses `waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)` to reap terminated children and track stopped/continued states without deadlocking the interactive loop.
- Manages terminal foreground process group ownership via `tcsetpgrp(STDIN_FILENO, pgid)` and safely reclaims terminal ownership when foreground jobs terminate or stop.
