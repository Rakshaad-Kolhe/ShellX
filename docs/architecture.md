# ShellX Technical Architecture

This document provides a comprehensive technical overview of the **ShellX** architecture, detailing the design, data structures, and execution flow of its core subsystems.

---

## Architecture Overview

ShellX is structured into four decoupled subsystems:

1. **Parser & AST Builder**: Converts raw user input strings into a sanitized linked list of `Command` abstractions.
2. **Built-in Command Engine**: Handles shell-internal state modifications (`cd`, `exit`) directly inside the parent process.
3. **Single Process Execution Engine**: Manages process creation (`fork`), standard stream manipulation (`dup2`), program execution (`execvp`), and parent synchronization (`waitpid`).
4. **Pipeline Inter-Process Communication (IPC) Subsystem**: Chains arbitrary numbers of child processes together via POSIX pipes (`pipe`).

---

## 1. Parser & Command AST Representation

The parser implementation resides in [src/parser.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/parser.c) and [include/parser.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/parser.h).

### Data Structure: `Command` Node

Every parsed command is represented by a heap-allocated `Command` node:

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

### Parsing Pipeline
1. **Whitespace Skipping**: `skip_whitespace` advances an internal input cursor past leading/interstitial spaces (`isspace`).
2. **Word Extraction**: `parse_word` extracts continuous non-whitespace, non-operator tokens into dynamically allocated character arrays (`copy_token`).
3. **Redirection Processing**: `parse_redirection` detects `<` (input), `>` (truncate output), and `>>` (append output) operators, storing target paths directly within the current `Command` structure.
4. **Pipeline Stage Creation**: When encountering the pipe operator `|`, the parser allocates a new `Command` structure via `allocate_command` and links it to `command->next`.
5. **Memory Lifecycle**: The lifecycle of the AST is completely managed. Callers invoke `free_command_list(Command *commands)`, which recursively traverses `next`, freeing `args[i]`, `input_path`, `output_path`, and node handles.

---

## 2. Process Execution Engine

The execution subsystem resides in [src/executor.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/executor.c) and [include/executor.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/executor.h).

### Child Process Spawning (`spawn_child`)

`spawn_child` abstracts process creation and file descriptor redirection for single commands and pipeline stages alike:

```c
pid_t spawn_child(const Command *command, int input_fd, int output_fd, int close_fd);
```

#### Child Process Execution Sequence (`pid == 0`)
1. **Descriptor Duplication**: If `input_fd != -1`, it is duplicated to `STDIN_FILENO` via `dup2`. If `output_fd != -1`, it is duplicated to `STDOUT_FILENO`.
2. **Unused Descriptor Cleanup**: `close_fd`, `input_fd`, and `output_fd` are closed in the child process to avoid file descriptor leaks.
3. **Explicit Redirection Application**: `apply_redirections` processes command-specific redirection files (`input_path`, `output_path`):
   - Input redirection: `open(path, O_RDONLY)` -> `dup2(fd, STDIN_FILENO)`.
   - Output redirection: `open(path, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0666)` -> `dup2(fd, STDOUT_FILENO)`.
4. **Program Loading**: `execvp(command->args[0], command->args)` replaces the child image with the target binary searched via system `PATH`.
5. **Failure Fallback**: If `execvp` fails (e.g., command not found), perror prints the error and `_exit(127)` terminates the child process immediately.

### Exit Status Collection
`execute_command` synchronizes with the child process using `waitpid`:
- `WIFEXITED(status)`: Returns `WEXITSTATUS(status)`.
- `WIFSIGNALED(status)`: Returns `128 + WTERMSIG(status)` in accordance with standard POSIX conventions.

---

## 3. Pipeline Model

The pipeline subsystem resides in [src/pipeline.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/pipeline.c) and [include/pipeline.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/pipeline.h).

### Iterative Pipeline Design

Rather than using recursive functions or holding all pipe descriptors open simultaneously, ShellX uses an **iterative rolling descriptor pattern**:

```
Stage 1 (cmd1)          Stage 2 (cmd2)          Stage 3 (cmd3)
  [Stdout] ---------------> [Stdin]
  Write to pipe_fds[1]     Read from prev_read_fd
                           Write to pipe_fds[1] ---> [Stdin]
                                                     Read from prev_read_fd
```

### Step-by-Step Pipeline Execution Flow
1. **Command Counting**: `count_commands` calculates total pipeline stages and allocates a `pids` array.
2. **Pipeline Loop**: For each `Command` node in the linked list:
   - If `command->next != NULL`, create a new POSIX pipe `pipe(pipe_fds)`.
   - Call `spawn_child` passing `previous_read_fd` as stdin and `pipe_fds[1]` as stdout. Pass `pipe_fds[0]` as `close_fd` so the child closes the unneeded pipe read end.
   - In the parent process:
     - Close `previous_read_fd` (the input pipe end for this stage).
     - Close `pipe_fds[1]` (the write pipe end for this stage).
     - Save `pipe_fds[0]` as `previous_read_fd` for the next stage.
3. **Parent Synchronization**: After spawning all stages, the parent loops over the `pids` array and calls `wait_for_child` on each PID.
4. **Exit Status propagation**: The exit status of the **final** pipeline stage is collected and returned as the pipeline exit status, fulfilling POSIX standard behavior.

---

## 4. Current System Limitations

The current implementation (`v0.1.0`) is deliberately constrained to core systems mechanisms. Known boundaries include:

- **Line Editing**: Uses `fgets` on standard stdin; advanced GNU Readline key-bindings (arrow navigation, line editing) are not present.
- **Built-in Scope**: Implements `cd` and `exit`. Environment variable setting (`export`), aliases, and job commands (`fg`/`bg`) are deferred.
- **Job Control & Signals**: `run_in_background` flag is populated when trailing `&` is parsed, but process group assignment (`setpgid`), terminal control transfers (`tcsetpgrp`), and async background monitoring (`SIGCHLD`) are scheduled for `v0.3.0`.
- **Fixed Argument Limit**: Commands are limited to `SHELLX_MAX_ARGS` (128 arguments).
