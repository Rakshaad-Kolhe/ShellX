#include "../include/jobs.h"
#include "../include/signals.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static void test_init_signals_and_pgid(void)
{
    init_signals();
    assert(get_shell_pgid() > 0);
}

static void test_give_terminal_to_self(void)
{
    pid_t shell_pgid = get_shell_pgid();
    give_terminal_to(shell_pgid);
}

int main(void)
{
    test_init_signals_and_pgid();
    test_give_terminal_to_self();

    printf("All signal tests passed.\n");
    return 0;
}
