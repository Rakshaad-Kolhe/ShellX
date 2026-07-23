# Contributing to ShellX

Thank you for your interest in contributing to **ShellX**! As a systems programming project, we prioritize technical accuracy, code readability, memory safety, and strict POSIX compliance.

This document outlines the workflow, coding style, and requirements for submitting contributions.

---

## 1. Development Environment Setup

### Prerequisites
To build and test ShellX, you need a POSIX-compliant UNIX environment (Linux, macOS, or Windows WSL) with the following toolchain installed:

- **C Compiler**: GCC (v9+) or Clang (v10+) supporting `-std=c17`.
- **Build System**: GNU Make.
- **Memory Testing (Recommended)**: Valgrind.

### Initializing the Workspace
Clone the repository and verify that your build environment compiles cleanly:

```bash
git clone https://github.com/your-username/ShellX.git
cd ShellX
make
make test
```

---

## 2. Build & Test Commands

We use GNU `make` for binary compilation and testing.

- **Build binary**: `make` (Outputs binary to `build/shellx`)
- **Run shell**: `make run`
- **Execute unit test suite**: `make test`
- **Clean build directory**: `make clean`
- **Rebuild from scratch**: `make rebuild`

### Memory Leak Validation
Before submitting pull requests, run the test binaries under Valgrind to ensure zero memory leaks and invalid accesses:

```bash
valgrind --leak-check=full ./build/test_parser
valgrind --leak-check=full ./build/test_executor
valgrind --leak-check=full ./build/test_builtins
valgrind --leak-check=full ./build/test_pipeline
```

---

## 3. C Coding Standards & Style Guide

ShellX adheres to strict C17 systems programming practices:

### Language Standard & Compiler Flags
- All source code must compile cleanly using `-std=c17 -Wall -Wextra -Wpedantic -g`.
- **Zero compiler warnings are allowed**.

### Formatting Conventions
- **Indentation**: 4 spaces (no tab characters).
- **Function Bracing**: Function opening braces start on a new line. Conditional and loop braces start on the same line.
- **Identifier Naming**:
  - Functions and variables use `snake_case` (e.g., `parse_command_line`, `input_path`).
  - Constants and Macros use `UPPER_SNAKE_CASE` (e.g., `SHELLX_MAX_ARGS`, `SHELLX_PARSER_H`).
  - Types use `PascalCase` or `typedef struct` naming (e.g., `Command`).
- **Header Guards**: All header files in `include/` must use standard `#ifndef SHELLX_<FILENAME>_H` guards.

### Systems Integrity Rules
1. **Explicit Return Code Checking**: Always check return values of system calls (`malloc`, `open`, `dup2`, `fork`, `pipe`, `waitpid`, `chdir`).
2. **File Descriptor Hygiene**: Ensure child processes and helper functions explicitly close unused pipe ends and file descriptors to avoid resource leaks or hangs.
3. **Memory Ownership**: Functions that borrow AST nodes must mark parameter pointers as `const Command *`. Allocation (`allocate_command`) and deallocation (`free_command_list`) responsibility must be explicit.

---

## 4. Commit Message Guidelines

We follow the **Conventional Commits** specification:

```text
<type>(<scope>): <short description>
```

### Allowed Types
- `feat`: A new user-facing shell feature or capability.
- `fix`: A bug fix in parser, executor, or pipeline logic.
- `docs`: Documentation updates or additions in `README.md` or `docs/`.
- `test`: Adding missing unit tests or refactoring test binaries.
- `refactor`: Code restructuring without functional changes.
- `chore`: Maintenance tasks (Makefile, `.gitignore`, build scripts).

### Examples
- `feat(parser): add append output redirection token parsing`
- `fix(executor): close unneeded pipe write fd in child process`
- `docs(architecture): add sequence diagram for pipeline execution`
- `test(pipeline): add 3-stage pipeline unit test case`

---

## 5. Pull Request & Review Process

1. **Create a Feature Branch**: Branch off `main` using a descriptive name (`git checkout -b feat/readline-integration`).
2. **Implement & Test**: Ensure code compiles with zero warnings and all unit tests pass (`make test`).
3. **Submit PR**: Open a pull request against `main` filling out the [Pull Request Template](.github/pull_request_template.md).
4. **Code Review**: A maintainer will review your PR. Address any requested changes promptly.
