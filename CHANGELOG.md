# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Planned
- POSIX job control, background job processing (`&`), and `jobs`/`fg`/`bg` built-ins (`v0.3.0`).
- Script file execution mode and environment variable expansion (`v0.4.0`).
- GitHub Actions CI matrix builds and Valgrind memory leak verification (`v1.0.0`).

---

## [0.2.0] - 2026-07-23

### Added
- **GNU Readline Integration**: Replaced `fgets()` input loop in `src/main.c` with GNU `readline()` for interactive line editing.
- **Configurable Prompt**: Added `ShellX$ ` prompt macro (`SHELLX_PROMPT`) in `include/shell.h`.
- **Persistent Command History**: Integrated `read_history()` and `write_history()` to persist history entries across sessions to `~/.shellx_history`.
- **Duplicate & Blank Filtering**: Implemented consecutive duplicate filtering (`is_duplicate_history`) and blank input skipping to keep history clean.
- **Clean EOF Handling**: Added graceful `Ctrl+D` (EOF) exit handling without memory leaks (`clear_history()`).
- **Build System Update**: Updated `Makefile` to link `-lreadline` and documented `libreadline-dev` system dependency in `README.md`.

---

## [0.1.0] - 2026-07-23

### Added
- **Lexical Parser & AST Subsystem**: Implemented `parse_command_line` and `free_command_list` in [src/parser.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/parser.c) for tokenizing input strings into linked `Command` nodes.
- **I/O Redirection**: Added support for input redirection (`<`), output truncation (`>`), and output appending (`>>`).
- **Pipeline Engine**: Implemented `execute_pipeline` in [src/pipeline.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/pipeline.c) using POSIX `pipe()`, `dup2()`, and iterative rolling descriptor management.
- **Process Execution Engine**: Implemented child process spawning and exit status collection in [src/executor.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/executor.c) via `fork`, `execvp`, and `waitpid`.
- **Built-in Command Engine**: Implemented `cd` (with `$HOME` fallback and argument checks) and `exit` in [src/builtins.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/builtins.c).
- **Unit Test Suite**: Developed modular unit test binaries (`test_parser`, `test_executor`, `test_builtins`, `test_pipeline`) integrated with `make test`.
- **Repository Engineering & Documentation**: Created professional `README.md`, technical architecture specifications (`docs/architecture.md`), design decisions (`docs/design-decisions.md`), future roadmap (`docs/future-roadmap.md`), contribution guidelines (`CONTRIBUTING.md`), issue templates, PR template, security policy (`SECURITY.md`), and MIT license.
