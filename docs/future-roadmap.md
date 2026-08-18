# ShellX Future Roadmap (v0.1.0 to v1.0.0)

This document outlines the planned development trajectory for **ShellX** from its initial baseline (`v0.1.0`) to a feature-complete, production-grade POSIX-oriented shell (`v1.0.0`).

---

## Release Timeline Overview

```
v0.1.0                 v0.2.0                 v0.3.0                 v0.4.0                 v0.5.0 (Current)       v1.0.0
  [Core Engine]  ---> [Readline & History] ---> [Job Control & Signals] ---> [Expansion & Built-ins] ---> [Startup & Aliases] ---> [Stable Release]
```

---

## Milestone Detail

### Version 0.1.0 — Core Shell Engine
- **Status**: Completed
- **Focus**: Core parsing, execution, redirection, and pipeline IPC.
- **Deliverables**:
  - Lexical scanner and linked list AST parser (`parse_command_line`).
  - Single command execution engine (`fork`, `dup2`, `execvp`, `waitpid`).
  - Standard file redirections: `<` input, `>` output truncate, `>>` output append.
  - Arbitrary-length multi-stage pipeline executor (`execute_pipeline`).
  - Essential built-ins (`cd`, `exit`).
  - Automated unit test suite (`make test`).

---

### Version 0.2.0 — Line Editing & Persistent History
- **Status**: Completed
- **Focus**: Enhancing the interactive shell user experience.
- **Deliverables**:
  - GNU Readline integration for interactive command line editing.
  - Configurable prompt macro.
  - History traversal with Up/Down arrow keys.
  - Persistent command history file (`~/.shellx_history`) with consecutive duplicate filtering.
  - Clean EOF exit handling (`Ctrl+D`).

---

### Version 0.3.0 — POSIX Job Control & Background Jobs
- **Status**: Completed
- **Focus**: Asynchronous process execution and terminal signal control.
- **Deliverables**:
  - Asynchronous background command execution (`&`).
  - Process group creation (`setpgid`) and shell process group isolation.
  - Terminal foreground control transfer (`tcsetpgrp`).
  - Asynchronous child reaping with `SIGCHLD` signal handler to avoid zombie processes.
  - Built-in job control commands: `jobs`, `fg`, `bg`.
  - Signal handling for `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z).

---

### Version 0.4.0 — Shell Expansion, Quoting & Environment Built-ins
- **Status**: Completed
- **Focus**: Lexical quoting, parameter expansions, and environment built-ins.
- **Deliverables**:
  - Full lexer with quote-aware scanner (`lexer.c`) separating quotes, escapes, and operators.
  - Single quoting (`'...'`) with literal text preservation.
  - Double quoting (`"..."`) with variable/escape expansion and operator shielding.
  - Backslash escaping (`\`) outside and inside double quotes.
  - Environment variable expansion (`$VAR`, `${VAR}`) and tilde expansion (`~`, `~/...`).
  - Special parameters: `$?` (last exit status) and `$$` (shell PID).
  - Built-in commands: `export`, `unset`, `env`, and `cd` with `PWD` tracking.
  - Pipeline subshell execution for built-ins in child stages.

---

### Version 0.5.0 — Shell Startup Configuration & Aliases (Current Release)
- **Status**: Completed
- **Focus**: Startup configuration file loading, alias management, and dynamic prompt.
- **Deliverables**:
  - Startup configuration loader (`config.c`) loading `$HOME/.shellxrc`.
  - Missing `.shellxrc` handling without error.
  - In-memory alias table (`alias.c`) with `alias` and `unalias` built-ins.
  - Bounded alias expansion with cycle detection and recursion depth limiting (`max_depth = 16`).
  - Dynamic prompt customization via `$PS1`.
  - Robust comment (`#`) parsing with quoted hash preservation.
  - Line-numbered diagnostic reporting for configuration errors.

---

### Version 1.0.0 — Production Quality & POSIX Conformance
- **Status**: Planned
- **Focus**: Script execution mode, CI workflows, and conformance validation.
- **Deliverables**:
  - Non-interactive script execution (`shellx script.sh`).
  - Command chaining with logical operators (`&&`, `||`, `;`).
  - GitHub Actions CI workflow covering GCC and Clang builds across Linux distributions.
  - Automated AddressSanitizer and Valgrind zero-leak validation in CI.
  - Conformance benchmarking against POSIX.1-2008 specifications.
