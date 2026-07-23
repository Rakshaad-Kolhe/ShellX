## Summary
Briefly explain the goal of this PR and what changes it introduces.

## Implementation Details
Describe the key architectural and code changes:
- Component(s) modified:
- Data structure / function changes:
- Standard / library dependencies:

## Testing Performed
Describe how you tested these changes:
- [ ] Ran automated test suite via `make test`
- [ ] Executed interactive manual tests in `shellx` REPL
- [ ] Ran memory check / leak inspection (e.g. `valgrind ./build/test_pipeline`)

```text
Paste test runner output or verification traces here
```

## Pull Request Checklist
- [ ] Code follows C17 standard (`-std=c17`) and compiles cleanly with zero warnings (`-Wall -Wextra -Wpedantic`).
- [ ] Header guards (`SHELLX_*_H`) are present in all new/modified header files.
- [ ] All public functions are documented in header files.
- [ ] Memory ownership contracts are maintained (no double frees or unclosed file descriptors).
- [ ] Updated or added corresponding unit tests in `tests/`.
- [ ] Documentation (`README.md`, `docs/`) updated if necessary.
- [ ] No functional regressions introduced to existing parser, executor, or pipeline logic.
