# Changelog

All notable changes to **ShellX** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.5.0] - 2026-08-18

### Added
- **Shell Startup Configuration (`~/.shellxrc`)**:
  - Implemented configuration loader (`src/config.c`, `include/config.h`) reading `$HOME/.shellxrc` once at startup.
  - Missing `.shellxrc` is handled silently and gracefully without error.
  - Supports environment assignments (`NAME=VALUE`, `NAME="VALUE"`), prompt customization (`PS1="Prompt$ "`), and alias definitions (`alias name="value"`).
  - Robust comment handling (`#`) with literal `#` character preservation inside quoted values.
  - Line-numbered diagnostic reporting (`shellx: ~/.shellxrc:<line>: <error>`) for syntax errors without crashing or corrupting shell state.
- **Internal Alias Subsystem (`src/alias.c`, `include/alias.h`)**:
  - Encapsulated in-memory alias table managing deep-copied string mappings.
  - `alias` built-in command: lists all aliases, prints specific alias definitions, and defines new/updated aliases.
  - `unalias` built-in command: removes specified aliases with usage validation.
  - Bounded alias expansion supporting pipeline segments and argument continuation with cycle detection and maximum recursion depth (`SHELLX_MAX_ALIAS_DEPTH = 16`).
- **Dynamic Prompt Customization (`get_prompt()`)**:
  - Runtime prompt evaluation checks `PS1` environment variable, falling back to `SHELLX_DEFAULT_PROMPT` (`ShellX$ `).
- **Automated Test Suite (`tests/test_config.c`)**:
  - Added 12 comprehensive unit tests for configuration loading, prompt overrides, alias CRUD operations, recursion protection, and quoted comments.

---

## [0.4.0] - 2026-08-18

### Added
- **Lexical Analyzer & Quote-Aware Tokenizer (`src/lexer.c`, `include/lexer.h`)**:
  - Dynamic `TokenList` and `StringBuilder` token scanner.
  - Single quoting (`'...'`) with literal text preservation.
  - Double quoting (`"..."`) with variable/escape expansion and operator shielding.
  - Backslash escaping (`\`) outside and inside double quotes.
  - Operator shielding: `|`, `<`, `>`, `>>`, and `&` emit literal `TOKEN_WORD` when quoted or escaped.
- **Parameter & Variable Expansion Engine (`src/expansion.c`, `include/expansion.h`)**:
  - Environment variable expansion (`$VAR`, `${VAR}`).
  - Special parameters: `$?` (exit code of last foreground command/pipeline) and `$$` (shell PID).
  - Tilde expansion (`~`, `~/...`) resolving leading `~` to `$HOME`.
  - POSIX identifier validation (`is_valid_identifier`).
- **Environment Management Built-in Commands (`src/builtins.c`, `include/builtins.h`)**:
  - `export [NAME[=VALUE] ...]`: Sets or lists exported variables with syntax validation.
  - `unset [NAME ...]`: Unsets specified environment variables.
  - `env`: Prints active environment variables.
  - `cd`: Updates `PWD` in the environment on directory change.
- **Pipeline Subshell Execution (`src/executor.c`)**:
  - `spawn_child` executes built-in commands inside child subshells when chained in pipelines.
- **Automated Test Suite (`tests/test_expansion.c`)**:
  - Added 11 comprehensive unit tests for quoting, escaping, variable expansion, and error cases.

---

## [0.3.0] - 2026-07-23

### Added
- **POSIX Job Control & Signal Handling (`src/jobs.c`, `src/signals.c`)**:
  - Background process execution (`&`).
  - Process group isolation via `setpgid()`.
  - Terminal ownership control via `tcsetpgrp()`.
  - Non-blocking asynchronous child reaping with `SIGCHLD` handler.
  - Built-in commands: `jobs`, `fg`, `bg`.
  - Signal handling for `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z).
- **Automated Test Suites (`tests/test_jobs.c`, `tests/test_signals.c`)**:
  - Unit tests for job table lifecycle, state transitions, and signal management.

---

## [0.2.0] - 2026-07-15

### Added
- **GNU Readline Interactive REPL (`src/main.c`)**:
  - Interactive line editing, history navigation (Up/Down), and prompt rendering.
  - Persistent command history stored in `~/.shellx_history` with duplicate filtering.
  - Clean EOF handling (`Ctrl+D`).

---

## [0.1.0] - 2026-07-01

### Added
- **Core Shell Engine & Execution Subsystem**:
  - Command parser (`src/parser.c`) and AST structure.
  - Single command executor (`src/executor.c`) with `fork`, `execvp`, `waitpid`.
  - File redirection (`<`, `>`, `>>`).
  - Arbitrary-length multi-stage process pipeline executor (`src/pipeline.c`).
  - Essential built-ins: `cd`, `exit`.
  - Unit test suite (`test_parser`, `test_executor`, `test_builtins`, `test_pipeline`).
