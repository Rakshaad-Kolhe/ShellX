#ifndef SHELLX_SIGNALS_H
#define SHELLX_SIGNALS_H

#include <sys/types.h>

/*
 * Initialize signal handlers for interactive shell job control.
 * Configures SIGINT, SIGTSTP, SIGTTIN, SIGTTOU, and SIGCHLD.
 */
void init_signals(void);

/*
 * Reset signal handlers in child process to default actions.
 */
void setup_child_signals(void);

/*
 * Transfer terminal foreground process group ownership to pgid.
 */
void give_terminal_to(pid_t pgid);

/*
 * Get the shell process group ID.
 */
pid_t get_shell_pgid(void);

/*
 * Return 1 if shell is running interactively on a TTY, 0 otherwise.
 */
int is_shell_interactive(void);

#endif
