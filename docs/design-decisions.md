# ShellX Design Decisions & Trade-offs

This document outlines key architectural design decisions, rationale, trade-offs, and invariants enforced across **ShellX**.

---

## 1. Startup Configuration vs General Scripting Engine

### Context
Shells require startup configuration (`~/.shellxrc`) to initialize environment variables, aliases, and prompts.

### Decision
Implement a dedicated, configuration-specific line parser (`src/config.c`) rather than treating `.shellxrc` as a generic executable shell script.

### Rationale
- **Security & Predictability**: Prevents executing arbitrary commands (`system()`, `popen()`, `/bin/sh -c`) during startup.
- **Maintainability**: Avoids entangling startup initialization with incomplete scripting control flow grammar (`if`, `while`, `for`).
- **Resilience**: Malformed lines report file and line numbers without aborting initialization or corrupting state.

---

## 2. Alias Table Architecture & Memory Ownership

### Context
Aliases map custom shorthand names to replacement strings and must persist across interactive commands.

### Decision
Encapsulate the alias table within `src/alias.c` using a singly-linked list with strict deep-copy memory ownership.

### Rationale
- **Encapsulation**: The internal node structure (`Alias`) is hidden from other subsystems.
- **Memory Safety**: Updating or removing an alias frees the previous strings immediately. `destroy_alias_table()` frees all nodes at shutdown, guaranteeing 0 Valgrind leaks.
- **Deterministic Listing**: `print_aliases()` sorts alias pointers with `qsort` to provide reproducible output.

---

## 3. Alias Expansion Ordering & Cycle Protection

### Context
Aliases can be self-referencing (e.g. `alias ls="ls --color"`) or mutually recursive (e.g. `alias a="b"`, `alias b="a"`).

### Decision
1. **Expansion Ordering**: Alias expansion occurs **first**, directly on the raw command line at command boundaries, before tokenization, parameter expansion, and parsing.
2. **Cycle Safety**: Expansion tracks visited alias identifiers within the current expansion chain and enforces a maximum depth limit of 16 (`SHELLX_MAX_ALIAS_DEPTH = 16`).
3. **Quoting Invariant**: Quoted or backslash-escaped command words (e.g. `\ll`, `'ll'`, `"ll"`) are strictly shielded from alias expansion.

---

## 4. Runtime Prompt Evaluation (`get_prompt()`)

### Context
Users require customizable interactive prompts via `$PS1`.

### Decision
Dynamically evaluate `$PS1` from the process environment on each REPL loop iteration, falling back to `SHELLX_DEFAULT_PROMPT` (`ShellX$ `).

### Rationale
- Unifies configuration from `.shellxrc` (`PS1="Custom$ "`), interactive export (`export PS1="New$ "`), and default settings into a single code path.

---

## 5. Subshell Built-in Execution in Pipelines

### Context
In multi-stage pipelines (`cmd1 | cmd2`), commands execute concurrently in child processes. When a built-in is placed in a pipeline (e.g. `env | grep PATH`), standard shell semantics dictate that its execution must not mutate the parent shell environment.

### Decision
Execute built-ins in child subshells when spawned as pipeline stages (`spawn_child`), while executing them in the parent process during standalone command execution.
