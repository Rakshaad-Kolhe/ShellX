# Systems Design Decisions & Rationale

This document documents the key architectural choices, engineering trade-offs, and design rationales behind **ShellX**.

---

## 1. Unified Quote-Aware Tokenization & Expansion

### Decision
Implement a unified, stateful lexical scanner (`src/lexer.c`) that performs quote stripping, backslash escape resolution, parameter expansion, and operator recognition in a single coherent scanning pipeline (`Input -> Lexer -> Parser -> AST -> Executor`).

### Alternatives Considered
- **Ad-hoc Pre-processing / Regex Replacement**: Replacing `$VAR` or stripping quotes using global string search-and-replace prior to tokenization.
- **Post-parse AST Expansion**: Building AST first and expanding variables in command arguments during execution.

### Rationale
- **Operator Shielding**: Quoted or escaped operators (`echo 'a | b'`, `echo ">"`, `echo foo\&bar`) must be recognized strictly as literal text (`TOKEN_WORD`), not as control operators. Pre-processing strings blindly or tokenizing before quoting leads to syntax corruption and command injection vulnerabilities.
- **Accurate Quoting Semantics**: POSIX quoting rules dictate that single quotes disable all expansion, double quotes enable variable expansion and specific backslash escapes (`\$`, `\"`, `\\`), and backslashes quote individual characters. A unified scanner maintains state accurately across adjacent quoted and unquoted substrings (e.g., `"Hello "'$USER'!`).

---

## 2. Explicit Shell Context for Special Parameters

### Decision
Encapsulate runtime shell parameters (`last_exit_status`, `shell_pid`) in a lightweight `ShellContext` structure passed explicitly to the parser and expansion engine:

```c
typedef struct ShellContext {
    int last_exit_status;
    pid_t shell_pid;
} ShellContext;
```

### Rationale
- **Reentrant & Deterministic Testing**: Parser and expansion functions remain pure and testable without relying on global mutable state or live process hooks.
- **Clean Execution Interface**: The REPL loop in `main()` captures the exit status of commands/pipelines and updates `context.last_exit_status` for the subsequent prompt cycle.

---

## 3. Strict POSIX Variable Identifier Validation

### Decision
Enforce strict identifier validation (`is_valid_identifier`) conforming to `[a-zA-Z_][a-zA-Z0-9_]*` for all variable expansions (`$NAME`, `${NAME}`) and environment built-ins (`export`, `unset`).

### Rationale
- **Predictable Lexing**: Delineates variable names from trailing punctuation and word characters (e.g., `$VAR/path`, `$USER_123`).
- **Defensive Environment Mutation**: Prevents malformed variable names (e.g., `1BAD=val`, `BAD-NAME=val`) from corrupting the C runtime environment table via `setenv()`.

---

## 4. Built-in Execution in Pipeline Subshells

### Decision
Allow built-in commands (`export`, `unset`, `env`, `cd`, `jobs`, `fg`, `bg`) to execute directly in child subshells when they appear as pipeline stages (`spawn_child` -> `execute_builtin`), while executing directly in the parent process for standalone commands.

### Rationale
- **Pipeline Composability**: Enables commands like `env | grep PATH` or `export | sort` to function naturally as pipeline sources.
- **Process Isolation**: In accordance with POSIX standards, built-in commands inside a pipeline execute in isolated child subshells; environment mutations (like `export FOO=bar | cat`) or directory changes (`cd /tmp | ls`) do not contaminate the parent shell process.

---

## 5. Linked Command AST Representation

### Decision
Represent pipelines as a singly linked list of `Command` structures, where each node points to the next pipeline stage (`Command *next`).

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

### Rationale
- **Dynamic Pipeline Sizing**: Linked nodes allow pipelines to scale to arbitrary lengths without requiring dynamic array reallocations during token parsing.
- **Natural Ownership Flow**: Memory allocation and deallocation mirror singly linked list traversals (`free_command_list`), making memory leaks easily preventable and verifiable via Valgrind.
- **Minimal Complexity**: For a POSIX pipeline shell, a linear linked list maps 1:1 to the execution flow of pipeline stages.

---

## 6. Decoupling Parsing from Execution

### Decision
Strictly separate string tokenization and AST parsing (`parse_command_line`) from process spawning and pipeline execution (`execute_pipeline`, `execute_command`).

### Rationale
- **Side-Effect Prevention**: Parsing validation checks for syntax errors (e.g., trailing pipe `ls |`, missing redirection targets `cat <`) before any process is forked or file opened.
- **Deterministic Unit Testing**: Parser functions take raw strings and return inspectable `Command` structures, allowing comprehensive unit testing without executing real binary commands or modifying system state.

---

## 7. Iterative Rolling Pipeline Execution

### Decision
Execute multi-stage pipelines using an **iterative loop** with a single rolling file descriptor (`previous_read_fd`) rather than a recursive execution model or pre-allocating all pipes.

### Rationale
- **Bounded Resource Consumption**: Pre-allocating all pipe descriptors upfront consumes $2(N-1)$ file descriptors concurrently. The rolling pattern ensures at most **2 pipe descriptors** are open simultaneously in the parent shell process.
- **Stack Safety**: Iterative loops guarantee $O(1)$ stack frame overhead regardless of pipeline depth.
- **Clean Descriptor Inheritance**: Passing `close_fd` to `spawn_child` guarantees child processes close adjacent pipe ends, preventing deadlocks.

---

## 8. Encapsulated Job Table Architecture

### Decision
Maintain an encapsulated internal Job Table (`include/jobs.h`, `src/jobs.c`) that tracks background processes and pipelines in a private linked list rather than exposing raw global mutable state.

### Rationale
- **Encapsulation**: Hides internal list structure behind clean accessor APIs (`add_job`, `find_job_by_id`, `find_job_by_pid`, `remove_job_by_id`, `destroy_job_table`).
- **Memory Safety Guarantee**: `destroy_job_table()` cleans up all allocated `Job` structures and command strings upon shell exit, ensuring zero Valgrind memory leaks.
