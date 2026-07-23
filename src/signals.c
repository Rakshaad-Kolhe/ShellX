#define _POSIX_C_SOURCE 200809L

#include "signals.h"


#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static pid_t shell_pgid = 0;
static int shell_is_interactive = 0;

void init_signals(void)
{
    shell_is_interactive = isatty(STDIN_FILENO);

    if (shell_is_interactive) {
        while (tcgetpgrp(STDIN_FILENO) != (shell_pgid = getpgrp())) {
            kill(-shell_pgid, SIGTTIN);
        }

        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            /* Handle gracefully if already process group leader */
        }

        tcsetpgrp(STDIN_FILENO, shell_pgid);
    } else {
        shell_pgid = getpgrp();
    }
}

void setup_child_signals(void)
{
    if (shell_is_interactive) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
    }
}

void give_terminal_to(pid_t pgid)
{
    if (shell_is_interactive && pgid > 0) {
        tcsetpgrp(STDIN_FILENO, pgid);
    }
}

pid_t get_shell_pgid(void)
{
    return shell_pgid;
}

int is_shell_interactive(void)
{
    return shell_is_interactive;
}
