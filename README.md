# ShellX — A POSIX-Oriented Unix Shell in C17

[![C17 Standard](https://img.shields.io/badge/C-17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![POSIX Process Model](https://img.shields.io/badge/POSIX-Process%20Model-green.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

**ShellX** is a modular, POSIX-oriented Unix shell implemented from scratch in **C17** using standard POSIX process, terminal, signal, and environment APIs. Built with clean architectural decoupling, strict memory ownership guarantees, and zero memory leaks under Valgrind, ShellX serves as both a functional interactive command shell and a modern systems programming reference.

---

## Key Features

- **Shell Startup Configuration (`~/.shellxrc`)**:
  - Automatically loads initialization settings from `$HOME/.shellxrc` before the interactive REPL starts.
  - Missing `.shellxrc` is handled silently and gracefully without error.
  - Supports environment assignments (`NAME=VALUE`, `NAME="VALUE"`), prompt customization (`PS1="Prompt$ "`), and alias definitions (`alias name="value"`).
  - Comment support (`#`) with preservation of `#` inside quoted strings.
  - Diagnostic reporting identifies file and line numbers for malformed lines without corrupting shell state or terminating the session.
- **Internal Alias Subsystem**:
  - In-memory alias table managing deep-copied alias definitions.
  - `alias`: Lists all aliases in canonical `alias name='value'` format.
  - `alias NAME`: Displays the specific alias definition.
  - `alias NAME=VALUE` / `alias NAME="VALUE"`: Defines or updates aliases in the parent process.
  - `unalias NAME ...`: Removes specified alias definitions.
  - **Bounded Alias Expansion**: Replaces unquoted command words with their alias text, supporting pipelines and command chaining with visited-alias cycle detection and recursion depth limiting (`max_depth = 16`).
- **Dynamic Prompt Customization**:
  - Runtime prompt evaluation checks `$PS1` in the environment, falling back to `ShellX$ ` when unset.
- **Lexical Tokenizer & Expansion Engine**:
  - Full lexer with quote-aware scanner separating quotes, escapes, parameter expansions, and operator recognition.
  - **Single Quoting (`'...'`)**: Treats all enclosed characters literally with zero variable interpolation or operator parsing.
  - **Double Quoting (`"..."`)**: Preserves literal whitespace and operators while expanding variables (`$VAR`, `$?`, `$$`) and respecting escape sequences (`\$`, `\"`, `\\`).
  - **Backslash Escaping (`\`)**: Outside quotes, quotes any single character. Inside double quotes, retains escape meaning exclusively for `$`, `"`, and `\\`; other escaped characters (e.g. `\n`, `\a`) are preserved literally.
  - **Tilde Expansion (`~`, `~/...`)**: Resolves leading `~` to `$HOME`.
  - **Special Parameters**: Supports `$?` (last foreground exit status) and `$$` (shell PID).
- **Environment Management Built-ins**:
  - `export [NAME[=VALUE] ...]`: Sets or lists exported environment variables with POSIX identifier validation (`[a-zA-Z_][a-zA-Z0-9_]*`).
  - `unset [NAME ...]`: Unsets specified environment variables.
  - `env`: Prints current environment variables.
  - `cd`: Updates `PWD` in the environment on directory change.
- **POSIX Job Control & Signal Management**:
  - Full support for background jobs (`&`), job table tracking, and built-ins `jobs`, `fg`, and `bg`.
  - Process group isolation via `setpgid()` and terminal ownership control via `tcsetpgrp()`.
  - Non-blocking `SIGCHLD` asynchronous zombie process reaping.
  - Proper signal masking and terminal handling for `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z).
- **Multi-Stage Process Pipelines**: Arbitrary command piping (`cmd1 | cmd2 | cmd3 | ...`) using an iterative rolling file descriptor model that caps concurrent descriptor usage to $O(1)$.
- **Standard File Redirections**: Input redirection (`<`), output truncation (`>`), and output appending (`>>`).
- **Interactive REPL with Persistent History**: GNU Readline integration featuring configurable prompt, cursor navigation, history traversal (Up/Down), and persistent disk storage in `~/.shellx_history` with duplicate filtering.
- **Strict Quality Standards**: Compiled with `-std=c17 -Wall -Wextra -Wpedantic` with zero warnings, zero undefined behavior, and 100% leak-free heap execution verified under Valgrind.

---

## Architectural Highlights

- [include/config.h](include/config.h) & [src/config.c](src/config.c): Configuration loader, `.shellxrc` line parser, and environment/prompt initialization.
- [include/alias.h](include/alias.h) & [src/alias.c](src/alias.c): Encapsulated alias table and cycle-safe alias expansion engine.
- [include/lexer.h](include/lexer.h) & [src/lexer.c](src/lexer.c): Lexical analyzer and scanner handling quote states, backslash escapes, variable expansion, and operator recognition.
- [include/expansion.h](include/expansion.h) & [src/expansion.c](src/expansion.c): Variable lookup, special parameter formatting (`$?`, `$$`), tilde expansion, and identifier validation.
- [include/parser.h](include/parser.h) & [src/parser.c](src/parser.c): AST parser consuming tokens into a linked list of `Command` structures.
- [include/pipeline.h](include/pipeline.h) & [src/pipeline.c](src/pipeline.c): Manages iterative pipe descriptor creation, process group assignment, and pipeline stage synchronization.
- [include/executor.h](include/executor.h) & [src/executor.c](src/executor.c): Spawns child processes, applies file redirections (`<`, `>`, `>>`), handles built-ins in subshells, and executes binaries via `execvp`.
- [include/builtins.h](include/builtins.h) & [src/builtins.c](src/builtins.c): Executes `cd`, `exit`, `jobs`, `fg`, `bg`, `export`, `unset`, `env`, `alias`, and `unalias` directly in the shell process.
- [include/jobs.h](include/jobs.h) & [src/jobs.c](src/jobs.c): Encapsulated thread-safe Job Table tracking background process groups and job states (`RUNNING`, `STOPPED`, `DONE`).
- [include/signals.h](include/signals.h) & [src/signals.c](src/signals.c): Signal handler registration, process group signal routing, and terminal ownership transfers.

---

## Repository Structure

```
ShellX/
|-- include/                   # C header files (Public interfaces & definitions)
|   |-- alias.h               # In-memory alias table & expansion prototypes
|   |-- builtins.h            # Built-in command declarations
|   |-- config.h              # Configuration loader & ~/.shellxrc parsing
|   |-- executor.h            # Process spawning & redirection interface
|   |-- expansion.h           # Environment expansion & identifier validation
|   |-- jobs.h                # Job table data structures & management APIs
|   |-- lexer.h               # Lexical scanner & token definitions
|   |-- parser.h              # Syntax parser & command AST builder
|   |-- pipeline.h            # Command AST structure & pipeline executor
|   |-- shell.h               # Prompt macro & history file constants
|   \-- signals.h             # POSIX signal handler registrations
|-- src/                       # Source implementation files
|   |-- alias.c               # Alias storage, listing, and expansion logic
|   |-- builtins.c            # Built-in command execution logic
|   |-- config.c              # ~/.shellxrc line-by-line configuration parser
|   |-- executor.c            # Process spawning, redirection, and execvp
|   |-- expansion.c           # Variable resolution, $?, $$, and tilde expansion
|   |-- jobs.c                # In-memory job table implementation
|   |-- lexer.c               # Quote-aware tokenizer & scanner
|   |-- main.c                # Interactive REPL entry point with Readline
|   |-- parser.c              # Token-to-AST parsing & memory management
|   |-- pipeline.c            # Iterative multi-process IPC pipeline engine
|   \-- signals.c             # Signal setup and asynchronous child reaping
|-- tests/                     # Automated unit and integration test suites
|   |-- test_builtins.c       # Tests for cd, exit, export, unset, env, alias, unalias
|   |-- test_config.c         # Tests for ~/.shellxrc, aliases, PS1, comments
|   |-- test_executor.c       # Tests for process spawning and redirections
|   |-- test_expansion.c      # Tests for variable expansion, quotes, and escapes
|   |-- test_jobs.c           # Tests for job table lifecycle and state transitions
|   |-- test_parser.c         # Tests for tokenization and syntax parsing
|   |-- test_pipeline.c       # Tests for IPC pipeline execution & pipe fds
|   \-- test_signals.c        # Tests for signal handlers and child status
|-- docs/                      # Architectural & design documentation
|   |-- architecture.md       # Technical subsystem breakdown
|   |-- design-decisions.md   # Architectural trade-offs & design choices
|   |-- future-roadmap.md     # Detailed roadmap to v1.0.0
|   |-- github-discussions.md # Discussion category framework
|   |-- labels.md             # Issue & PR label reference
|   \-- milestones.md         # Milestone definitions
|-- Makefile                   # Build automation rules
|-- README.md                  # Project overview & documentation
\-- CHANGELOG.md               # Version history
```

---

## Build Instructions

### Prerequisites
- **Operating System**: Linux or POSIX-compliant UNIX environment (including WSL on Windows).
- **Compiler**: GCC or Clang supporting `-std=c17`.
- **Build System**: GNU Make.
- **Development Library**: GNU Readline development headers (`libreadline-dev` / `readline-devel`).

#### Installing Dependencies (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install build-essential libreadline-dev valgrind
```

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
# Or directly:
./build/shellx
```

### Running the Test Suite
ShellX features 8 automated test suites validating every subsystem:
```bash
make test
```

---

## Example Terminal Sessions

### 1. Startup Configuration (`~/.shellxrc`)
```text
# Content of ~/.shellxrc:
PROJECT=ShellX
PS1="ShellX[custom]$ "
alias ll="ls -la"
alias gs="git status"

# Starting ShellX automatically loads ~/.shellxrc:
$ ./build/shellx
ShellX[custom]$ echo $PROJECT
ShellX
ShellX[custom]$ ll /tmp
total 48
...
ShellX[custom]$ gs
On branch main
...
```

### 2. Managing Aliases
```text
ShellX$ alias c="clear"
ShellX$ alias
alias c='clear'
alias gs='git status'
alias ll='ls -la'
ShellX$ unalias c
```

### 3. Quoting, Escaping & Operator Shielding
```text
ShellX$ echo 'hello | world'
hello | world
ShellX$ echo "Quotes protect > and & operators"
Quotes protect > and & operators
ShellX$ echo Escaped\ \|\ Operator
Escaped | Operator
```

### 4. Environment Variables & Special Parameters
```text
ShellX$ export VERSION=1.0
ShellX$ echo "Building ShellX version $VERSION"
Building ShellX version 1.0
ShellX$ echo "Shell PID: $$"
Shell PID: 4522
ShellX$ false
ShellX$ echo "Last Exit Status: $?"
Last Exit Status: 1
```

### 5. Pipeline & File Redirection
```text
ShellX$ echo "Line 1\nLine 2\nLine 3" > data.txt
ShellX$ cat < data.txt | grep "Line 2"
Line 2
```

### 6. POSIX Job Control
```text
ShellX$ sleep 30 &
[1] 4612
ShellX$ jobs
[1]  4612 Running  sleep 30 &
ShellX$ fg 1
sleep 30
```

---

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
