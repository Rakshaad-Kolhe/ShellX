# ShellX Future Roadmap (v0.1.0 to v1.0.0)

This document outlines the planned development trajectory for **ShellX** from its initial baseline (`v0.1.0`) to a feature-complete, production-grade POSIX shell (`v1.0.0`).

---

## Release Timeline Overview

```
v0.1.0                 v0.2.0                 v0.3.0                 v0.4.0 (Current)       v0.5.0                 v1.0.0
  [Core Engine]  ---> [Readline & History] ---> [Job Control & Signals] ---> [Expansion & Built-ins] ---> [Script Execution] ---> [Stable Release]
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
  - Configurable `ShellX$ ` prompt macro.
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

### Version 0.4.0 — Shell Expansion, Quoting & Environment Built-ins (Current Release)
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

### Version 0.5.0 — Scripting Support & Non-Interactive Mode
- **Status**: Planned
- **Focus**: Non-interactive file execution and compound commands.
- **Deliverables**:
  - Non-interactive script execution (`shellx filename.sh`).
  - Command chaining with logical operators (`&&`, `||`, `;`).
  - Dynamic heap argument allocation replacing static `SHELLX_MAX_ARGS` limit.
  - Command substitution (`$(command)` / ```command```).

---

### Version 1.0.0 — Production Quality & POSIX Compliance
- **Status**: Planned
- **Focus**: Production hardening, CI workflows, and standard validation.
- **Deliverables**:
  - GitHub Actions CI workflow covering GCC and Clang builds across Linux distributions.
  - AddressSanitizer and Valgrind zero-leak validation in CI.
  - Conformance test suite benchmarking against POSIX.1-2008 specifications.
  - Complete API documentation and developer guides.
