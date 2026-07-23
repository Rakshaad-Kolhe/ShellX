# ShellX Future Roadmap (v0.1.0 to v1.0.0)

This document outlines the planned development trajectory for **ShellX** from its current baseline (`v0.1.0`) to a feature-complete, production-grade POSIX shell (`v1.0.0`).

---

## Release Timeline Overview

```
v0.1.0 (Current)       v0.2.0                 v0.3.0                 v0.4.0                 v1.0.0
  [Core Engine]  ---> [Readline & History] ---> [Job Control & Signals] ---> [Scripting & Vars] ---> [Stable Release]
```

---

## Milestone Detail

### Version 0.1.0 — Core Shell Engine (Current Implementation)
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
- **Status**: Planned
- **Focus**: Enhancing the interactive shell user experience.
- **Deliverables**:
  - Integrate GNU Readline or BSD Editline for line editing.
  - Implement arrow key navigation (history traversal, cursor movement).
  - Add persistent command history file (`~/.shellx_history`).
  - Add tab completion for PATH executables and local file system paths.
  - Handle `SIGINT` (Ctrl+C) during input to cancel current line without terminating shell.

---

### Version 0.3.0 — POSIX Job Control & Background Jobs
- **Status**: Planned
- **Focus**: Asynchronous process execution and terminal signal control.
- **Deliverables**:
  - Asynchronous background command execution (`&`).
  - Process group creation (`setpgid`) and shell process group isolation.
  - Terminal foreground control transfer (`tcsetpgrp`).
  - Asynchronous child reaping with `SIGCHLD` signal handler to avoid zombie processes.
  - Implement job control built-in commands: `jobs`, `fg`, `bg`.
  - Handle `SIGTSTP` (Ctrl+Z) to suspend foreground processes.

---

### Version 0.4.0 — Environment Expansion & Scripting
- **Status**: Planned
- **Focus**: Non-interactive file execution and variable evaluation.
- **Deliverables**:
  - Non-interactive script execution mode (`shellx filename.sh`).
  - Environment variable expansion (`$VAR`, `$HOME`, `$PATH`).
  - Last process exit status expansion (`$?`).
  - Built-in commands: `export`, `unset`, `pwd`, `echo`.
  - Dynamic heap argument allocation replacing static `SHELLX_MAX_ARGS` limit.

---

### Version 1.0.0 — Production Quality & POSIX Compliance
- **Status**: Planned
- **Focus**: Production hardening, CI workflows, and standard validation.
- **Deliverables**:
  - GitHub Actions CI workflow covering GCC and Clang builds across Linux distributions.
  - AddressSanitizer and Valgrind zero-leak validation in CI.
  - Conformance test suite benchmarking against POSIX.1-2008 specifications.
  - Complete API documentation and developer guides.
