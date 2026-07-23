# Repository Milestones

This document specifies the release milestones for **ShellX**.

---

## Milestone Taxonomy

| Milestone | Target Scope | Status |
| :--- | :--- | :---: |
| **`v0.1.0` — Core Shell Engine** | Baseline parsing, redirections (`<`, `>`, `>>`), arbitrary pipelines (`\|`), built-ins (`cd`, `exit`), and unit test suite. | **Completed** |
| **`v0.2.0` — Readline & History** | GNU Readline / BSD Editline integration, history file persistence (`~/.shellx_history`), and path autocompletion. | **Planned** |
| **`v0.3.0` — POSIX Job Control** | Background jobs (`&`), process groups (`setpgid`), terminal control (`tcsetpgrp`), signal handlers (`SIGCHLD`, `SIGINT`), and `jobs`/`fg`/`bg` built-ins. | **Planned** |
| **`v0.4.0` — Production Quality** | Script file execution mode, environment variable expansion (`$VAR`), `$?` status tracking, and dynamic buffer allocation. | **Planned** |
| **`v1.0.0` — Stable Release** | GitHub Actions CI workflow, Valgrind memory leak verification, and POSIX compliance test suite. | **Planned** |
