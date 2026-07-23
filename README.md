# ShellX

A lightweight, POSIX-compliant UNIX shell written in modern C17, designed as an educational model for UNIX systems programming, process management, inter-process communication (IPC), and I/O redirection.

[![C17](https://img.shields.io/badge/Language-C17-blue.svg)](https://en.cppreference.com/w/c/17)
[![Linux](https://img.shields.io/badge/Platform-Linux%20%2F%20POSIX-f05133.svg)](https://www.kernel.org)
[![POSIX](https://img.shields.io/badge/Standard-POSIX.1--2008-orange.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Build: GNU Make](https://img.shields.io/badge/Build-GNU%20Make-brightgreen.svg)](Makefile)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Status: Development](https://img.shields.io/badge/Status-Development-lightgrey.svg)](#roadmap)

---

## Overview

**ShellX** is a minimalist UNIX shell implementation focused on foundational systems programming concepts. It serves as an architectural blueprint for understanding process management, memory safety, lexical tokenization, file descriptor manipulation, and pipeline synchronization.

### Why ShellX Exists
Modern shells like `bash` and `zsh` consist of hundreds of thousands of lines of legacy code, making them difficult to study for core operating system mechanisms. **ShellX** strips away complex interactive features (e.g., dynamic line editing, alias expansion, parameter expansion) to highlight clean, unadorned UNIX kernel primitives (`fork`, `execvp`, `pipe`, `dup2`, `waitpid`).

### Primary Objectives
- **Systems Programming Rigor**: Showcase strict C17 standards compliance (`-std=c17 -Wall -Wextra -Wpedantic`), robust memory handling, and explicit error checking.
- **IPC & Process Architecture**: Provide a clear implementation of arbitrary-length IPC pipelines using standard file descriptor chaining.
- **Educational Clarity**: Maintain clean module separation between lexical parsing, pipeline AST abstraction, built-in command handling, and process execution.

---

## Features

The feature matrix below details the current implementation state of **ShellX**.

| Feature | Status | Implementation Details |
| :--- | :---: | :--- |
| **Command Parsing** | ✔ Implemented | Custom whitespace tokenizer & AST builder separating syntax parsing from execution. |
| **Built-in Commands** | ✔ Implemented | In-process execution for core built-ins (`cd` with `HOME` fallback, `exit`). |
| **Arbitrary Pipelines** | ✔ Implemented | Multi-stage pipeline execution (`cmd1 \| cmd2 \| ... \| cmdN`) via `pipe()` and `dup2()`. |
| **Input Redirection** | ✔ Implemented | File input redirection using `<` operator (`O_RDONLY`). |
| **Output Redirection** | ✔ Implemented | File output truncation redirection using `>` operator (`O_WRONLY \| O_CREAT \| O_TRUNC`). |
| **Append Redirection** | ✔ Implemented | File output append redirection using `>>` operator (`O_WRONLY \| O_CREAT \| O_APPEND`). |
| **Automated Test Suite** | ✔ Implemented | Comprehensive unit test suite covering parser, executor, built-in, and pipeline modules. |
| **GNU Readline / Line Editing** | ❌ Planned | Scheduled for milestone `v0.2.0`. |
| **Persistent History** | ❌ Planned | Scheduled for milestone `v0.2.0`. |
| **POSIX Job Control (`fg`/`bg`)** | ❌ Planned | Scheduled for milestone `v0.3.0`. |

> [!NOTE]
> Background command syntax (`&`) sets an internal parser flag (`run_in_background`), but asynchronous job control process management is scheduled for `v0.3.0`.

---

## Architecture

ShellX follows a modular compilation architecture where execution flow is divided into four main layers:

```mermaid
flowchart TD
    User([User Input String]) --> Parser[Parser Module<br/>parse_command_line]
    Parser --> AST[Command Linked List AST<br/>Command struct]
    AST --> Router{Is Built-in or Pipeline?}
    Router -->|Pipeline / Multi-stage| Pipeline[Pipeline Engine<br/>execute_pipeline]
    Router -->|Built-in cd / exit| Builtin[Built-in Engine<br/>execute_builtin]
    Router -->|Single Process| Executor[Execution Engine<br/>execute_command]
    Pipeline --> Processes[Forked Child Processes<br/>pipe, dup2, execvp]
    Executor --> Processes
    Builtin --> ShellProcess[ShellX Process State]
    Processes --> Wait[Process Synchronization<br/>waitpid status collection]
```

### Module Responsibilities
- [include/parser.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/parser.h) & [src/parser.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/parser.c): Tokenizes stdin input into null-terminated argument arrays and builds a linked list of `Command` structures.
- [include/pipeline.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/pipeline.h) & [src/pipeline.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/pipeline.c): Manages iterative pipe descriptor creation, child process spawns, descriptor inheritance, and exit status collection.
- [include/executor.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/executor.h) & [src/executor.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/executor.c): Spawns individual child processes, applies file redirections (`<`, `>`, `>>`), and executes binaries via `execvp`.
- [include/builtins.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/builtins.h) & [src/builtins.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/builtins.c): Executes shell state modification commands directly within the main shell process.

---

## Repository Structure

```
ShellX/
├── include/                   # C header files (Public interfaces & definitions)
│   ├── builtins.h            # Built-in command functions (cd, exit)
│   ├── executor.h            # Process spawning & redirection interface
│   ├── parser.h              # Syntax parser & command list builder
│   └── pipeline.h            # Command AST structure & pipeline executor
├── src/                       # Source implementation files
│   ├── builtins.c            # cd and exit execution logic
│   ├── executor.c            # fork, dup2, execvp, waitpid implementation
│   ├── main.c                # Interactive REPL entry point
│   ├── parser.c              # Lexical parser & memory lifecycle functions
│   └── pipeline.c            # Iterative multi-process IPC pipeline engine
├── tests/                     # Unit test suite binaries & sources
│   ├── test_builtins.c       # Tests for cd, exit, and environment fallbacks
│   ├── test_executor.c       # Tests for process spawning and redirections
│   ├── test_parser.c         # Tests for tokenization and syntax parsing
│   └── test_pipeline.c       # Tests for IPC pipeline execution & pipe fds
├── docs/                      # Architectural & design documentation
│   ├── architecture.md       # Technical subsystem breakdown
│   ├── design-decisions.md   # Architectural trade-offs & design choices
│   ├── future-roadmap.md     # Detailed roadmap to v1.0.0
│   ├── github-discussions.md # Discussion category framework
│   ├── labels.md             # Issue & PR label reference
│   └── milestones.md         # Milestone definitions
├── .github/                   # Issue forms and PR templates
│   ├── ISSUE_TEMPLATE/       # Bug report, feature, and doc templates
│   └── pull_request_template.md
├── CHANGELOG.md               # Version history (Keep a Changelog 1.0.0)
├── CODE_OF_CONDUCT.md         # Contributor Covenant Code of Conduct
├── CONTRIBUTING.md            # Development setup & contribution guide
├── LICENSE                    # MIT Open Source License
├── Makefile                   # Build automation rules
├── README.md                  # Project overview & documentation
└── SECURITY.md                # Vulnerability disclosure policies
```

---

## Build Instructions

### Prerequisites
- **Operating System**: Linux or POSIX-compliant UNIX environment (including WSL on Windows).
- **Compiler**: GCC or Clang supporting `-std=c17`.
- **Build System**: GNU Make.

### Compilation
To compile the shell binary:
```bash
make
```
The compiled executable is created at `build/shellx`.

### Running ShellX
Launch the interactive shell session:
```bash
make run
```
Or execute the built binary directly:
```bash
./build/shellx
```

### Running the Test Suite
ShellX features an automated test runner validating parser tokenization, redirection flags, built-ins, and multi-stage pipeline execution:
```bash
make test
```

### Rebuilding & Cleaning
```bash
make clean    # Removes the build/ directory and generated binaries
make rebuild  # Performs a clean build from scratch
```

---

## Example Terminal Sessions

Below are actual output sessions demonstrating **ShellX** supported capabilities.

### 1. Simple Commands & Built-in Operations
```text
shellx> echo hello
hello
shellx> cd tests
shellx> exit
```

### 2. Pipeline Execution
```text
shellx> printf hello | grep hello
hello
shellx> seq 1 10 | tail -n 3
8
9
10
```

### 3. File Input & Output Redirection
```text
shellx> echo "ShellX Production Release" > output.txt
shellx> cat < output.txt
ShellX Production Release
shellx> echo "Appending extra line" >> output.txt
shellx> cat < output.txt | grep Appending
Appending extra line
```

---

## Roadmap

- [x] **Phase 1 — Core Shell Engine (v0.1.0)**
  - [x] Command line tokenization and whitespace handling
  - [x] Command linked list structure allocation and destruction
  - [x] Input (`<`), Output (`>`), and Append (`>>`) redirections
  - [x] Multi-stage process pipelines (`cmd1 | cmd2 | cmd3`)
  - [x] In-process built-ins (`cd`, `exit`)
  - [x] Automated unit test suite with 100% test pass rate
- [ ] **Phase 2 — Line Editing & Persistent History (v0.2.0)**
  - [ ] Integration with GNU Readline / BSD Editline
  - [ ] Arrow key navigation and line editing
  - [ ] Persistent command history stored in `~/.shellx_history`
- [ ] **Phase 3 — POSIX Job Control & Signal Handling (v0.3.0)**
  - [ ] Background job execution (`&`) with process table tracking
  - [ ] Process group creation (`setpgid`) and terminal control (`tcsetpgrp`)
  - [ ] Foreground (`fg`) and background (`bg`) built-in commands
  - [ ] Custom signal handlers (`SIGINT`, `SIGTSTP`, `SIGCHLD`)
- [ ] **Phase 4 — Production Refactoring & Scripting Support (v0.4.0)**
  - [ ] Non-interactive script file execution (`shellx script.sh`)
  - [ ] Environment variable expansion (`$VAR`) and exit code `$?` status
  - [ ] Dynamic heap buffer expansion for arbitrary command lengths
- [ ] **Phase 5 — Stable v1.0 Release (v1.0.0)**
  - [ ] Continuous Integration (CI) test workflows
  - [ ] Valgrind leak-free validation suite
  - [ ] Full POSIX shell conformance documentation

---

## Contributing

Contributions are welcome! Please review [CONTRIBUTING.md](CONTRIBUTING.md) for environment setup, coding style standards, commit conventions, and testing requirements before submitting pull requests.

---

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
