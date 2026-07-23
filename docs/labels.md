# Repository Issue & Pull Request Labels

This document specifies the standard label taxonomy for organizing issues, pull requests, and milestones in the **ShellX** repository.

---

## Category Labels

| Label Name | Hex Color | Description |
| :--- | :---: | :--- |
| `bug` | `#d73a4a` | Indicates a bug or unexpected error in existing shell code. |
| `enhancement` | `#a2eeef` | New feature or improvement request. |
| `good first issue` | `#7057ff` | Suitable for contributors new to ShellX or systems programming. |
| `documentation` | `#0075ca` | Improvements or additions to README, docs/, or inline comments. |
| `refactor` | `#cfd3d7` | Code changes that improve structure without altering behavior. |
| `performance` | `#d4c5f9` | Memory efficiency or execution speed optimizations. |
| `testing` | `#fbca04` | Additions or fixes to unit tests (`tests/`). |

---

## Subsystem & Technical Labels

| Label Name | Hex Color | Description |
| :--- | :---: | :--- |
| `parser` | `#bfd4f2` | Pertains to tokenization, parsing (`parser.c`), or syntax AST. |
| `executor` | `#bfdadc` | Pertains to process spawning (`executor.c`), `fork`, `execvp`, or `dup2`. |
| `signals` | `#e99695` | Pertains to signal handling, process groups, or job control. |
| `OS` | `#1d76db` | POSIX standard compatibility, Linux-specific, or BSD/macOS portability issues. |
