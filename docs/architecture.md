# ShellX Technical Architecture

This document provides a comprehensive technical overview of the **ShellX** architecture, detailing the design, data structures, and execution flow of its core subsystems.

---

## Architecture Overview

ShellX is structured into five decoupled subsystems:

1. **Parser & AST Builder**: Converts raw user input strings into a sanitized linked list of `Command` abstractions.
2. **Built-in Command Engine**: Handles shell-internal state modifications (`cd`, `exit`) directly inside the parent process.
3. **Single Process Execution Engine**: Manages process creation (`fork`), standard stream manipulation (`dup2`), program execution (`execvp`), and parent synchronization (`waitpid`).
4. **Pipeline Inter-Process Communication (IPC) Subsystem**: Chains arbitrary numbers of child processes together via POSIX pipes (`pipe`).
5. **Job Management Subsystem**: Maintains an in-memory Job Table tracking background processes and pipelines across shell sessions.

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
`execute_command` synchronizes with foreground child processes using `wait_for_child`:
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

---

## 4. Job Management Subsystem

The Job Management subsystem resides in [src/jobs.c](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/src/jobs.c) and [include/jobs.h](file:///wsl.localhost/Ubuntu/home/rakshaad/projects/ShellX/include/jobs.h).

### Data Structure: `Job`

```c
typedef enum JobState {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct Job {
    int job_id;               /* 1-based sequential job ID */
    pid_t pgid;               /* Process Group ID (leader PID) */
    pid_t pid;                /* Leader process PID */
    char *command;            /* Reconstructed command string */
    JobState state;           /* JobState enum */
    int is_background;        /* 1 for background job, 0 for foreground */
    struct Job *next;         /* Pointer to next job in internal list */
} Job;
```

### Job Table Responsibilities
- `init_job_table()`: Initializes the internal job list and resets job ID counter to 1.
- `add_job(pid, pgid, command_str, is_bg)`: Allocates a new `Job` struct, assigns a sequential 1-based `job_id`, formats the command string (`format_command_string`), and registers it in the internal job table.
- `find_job_by_id(job_id)` / `find_job_by_pid(pid)`: Searches the internal job table by job ID or leader PID.
- `remove_job_by_id(job_id)` / `remove_completed_jobs()`: Removes and frees specified or completed job nodes.
- `destroy_job_table()`: Frees all allocated jobs and command strings upon shell shutdown for zero Valgrind memory leaks.

---

## 5. Current System Limitations

The current implementation (`v0.3.0-alpha`) tracks background jobs internally in the Job Table. Future job control capabilities scheduled for `v0.3.0` include:

- **User-facing Job Built-ins**: `jobs`, `fg`, and `bg` CLI commands.
- **Process Groups & Terminal Control**: `setpgid` process group creation and `tcsetpgrp` terminal ownership transfers.
- **Asynchronous Child Reaping**: `SIGCHLD` signal handler to automatically reap completed background processes and prevent zombie accumulation.
