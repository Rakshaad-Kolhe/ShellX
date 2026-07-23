# Systems Design Decisions & Rationale

This document documents the key architectural choices, engineering trade-offs, and design rationales behind **ShellX**.

---

## 1. Linked Command AST Representation

### Decision
Represent pipelines as a single-linked list of `Command` structures, where each node points to the next pipeline stage (`Command *next`).

```c
struct Command {
    char *args[SHELLX_MAX_ARGS];
    size_t arg_count;
    char *input_path;
    char *output_path;
    int append_output;
    int run_in_background;
    struct Command *next;
};
```

### Alternatives Considered
- **Flat Dynamic Array of Commands**: Storing commands in a dynamic array (`Command *cmd_array`, `size_t count`).
- **Full Parse Tree / AST**: Building a tree supporting nested expressions, subshells `(...)`, and logical operators (`&&`, `||`).

### Rationale
- **Dynamic Pipeline Sizing**: Linked nodes allow pipelines to scale to arbitrary lengths without requiring dynamic array reallocations during token parsing.
- **Natural Ownership Flow**: Memory allocation and deallocation mirror singly linked list traversals (`free_command_list`), making memory leaks easily preventable and verifiable via Valgrind.
- **Minimal Complexity**: For a POSIX pipeline shell, a linear linked list maps 1:1 to the execution flow of pipeline stages.

---

## 2. Decoupling Parsing from Execution

### Decision
Strictly separate string tokenization and AST parsing (`parse_command_line`) from process spawning and pipeline execution (`execute_pipeline`, `execute_command`).

```
[Raw String Input] ---> parse_command_line() ---> [Command AST] ---> execute_pipeline()
```

### Rationale
- **Side-Effect Prevention**: Parsing validation checks for syntax errors (e.g., trailing pipe `ls |`, missing redirection targets `cat <`) before any process is forked or file opened. This prevents invalid syntax from producing partial side-effects like truncating existing files.
- **Deterministic Unit Testing**: Parser functions take raw strings and return inspectable `Command` structures, allowing comprehensive unit testing without executing real binary commands or modifying system state.
- **Clean Memory Management**: If syntax validation fails midway through parsing, the parser simply calls `free_command_list` on the partially constructed AST and returns `NULL`.

---

## 3. Iterative Rolling Pipeline Execution

### Decision
Execute multi-stage pipelines using an **iterative loop** with a single rolling file descriptor (`previous_read_fd`) rather than a recursive execution model or pre-allocating all pipes.

### Alternatives Considered
- **Recursive Execution**: Spawning pipeline stages via recursive function calls.
- **Pre-allocated Pipe Array**: Creating $N-1$ pipe descriptor pairs upfront in an array `int fds[N-1][2]`.

### Rationale
- **Bounded Resource Consumption**: Pre-allocating all pipe descriptors upfront consumes $2(N-1)$ file descriptors concurrently. If $N$ is large, the process risks hitting the system file descriptor limit (`RLIMIT_NOFILE`). The rolling pattern ensures at most **2 pipe descriptors** are open simultaneously in the parent shell process.
- **Stack Safety**: Iterative loops guarantee $O(1)$ stack frame overhead regardless of pipeline depth.
- **Clean Descriptor Inheritance**: Passing `close_fd` to `spawn_child` guarantees that child processes close the write/read ends of adjacent pipes, preventing deadlocks caused by unclosed write descriptors keeping readers waiting indefinitely.

---

## 4. Borrowed Pointer Memory Ownership Contract

### Decision
Functions in the execution engine (`execute_command`, `spawn_child`, `execute_pipeline`, `is_builtin`, `execute_builtin`) take **read-only borrowed pointers** (`const Command *`).

```c
int execute_command(const Command *command);
int execute_pipeline(const Command *commands);
int is_builtin(const Command *command);
```

### Rationale
- **Explicit Lifetime Separation**: The execution engine does not alter, reallocate, or free the AST passed to it.
- **Single Responsibility Memory Lifecycle**: Memory allocation occurs exclusively inside `parse_command_line`, and memory freeing occurs exclusively inside `free_command_list` at the end of the REPL iteration loop in `main()`.

---

## 5. Standard POSIX Exit Status Encoding

### Decision
Encode process exit statuses in exact compliance with POSIX standard shell conventions:

```c
if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
}
if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
}
```

### Special Exit Codes
- **`127`**: Returned when binary execution fails (`execvp` error, command not found).
- **`126`**: Returned when child environment setup or redirection file opening fails prior to execution.
