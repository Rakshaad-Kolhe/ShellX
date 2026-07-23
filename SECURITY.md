# Security Policy

The **ShellX** maintainers take security and systems reliability seriously. This document outlines our supported versions and security vulnerability reporting process.

---

## Supported Versions

Security updates are applied to the latest stable and active development releases:

| Version | Supported |
| :--- | :---: |
| `0.1.x` | :white_check_mark: Supported |
| `< 0.1.0` | :x: Unsupported |

---

## Reporting a Vulnerability

If you discover a security vulnerability, buffer overflow, file descriptor leak, or memory safety defect in ShellX, please report it responsibly.

> [!CAUTION]
> **Do NOT open a public GitHub issue for security vulnerabilities or unpatched security exploits.**

### Reporting Procedure
1. Send a detailed vulnerability report via email to security maintainers at **security@shellx-project.org** (or contact maintainers directly via private channel).
2. Include the following details:
   - Type of issue (e.g. buffer overflow, format string vulnerability, unclosed file descriptor leak, privilege issue).
   - Proof of concept input string or steps to trigger the defect.
   - Affected version(s) and operating system environment.
   - Proposed patch or mitigation if available.

### Response Timeframe
- **Initial Acknowledgment**: Within 48 hours of report submission.
- **Triage & Severity Assessment**: Within 5 business days.
- **Patch Release & Advisory**: Priority release for verified vulnerabilities before public disclosure.

---

## Security Principles in ShellX

- **Explicit Memory Bounds**: Strict bounds checking on argument arrays (`SHELLX_MAX_ARGS`).
- **Safe Environment Subprocess Spawning**: Using `execvp` with null-terminated string arrays rather than unsafe `system()` shell command strings.
- **Descriptor Hygiene**: Explicit file descriptor cleanup (`close`) to prevent descriptor leaks across subprocess calls.
