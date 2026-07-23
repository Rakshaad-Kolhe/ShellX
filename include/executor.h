#ifndef SHELLX_EXECUTOR_H
#define SHELLX_EXECUTOR_H

#include "jobs.h"
#include "pipeline.h"

#include <sys/types.h>

/*
 * Close a file descriptor if it is not -1.
 */
void close_if_open(int fd);

/*
 * Execute one foreground or background command.
 * The command is borrowed and must not be modified or freed.
 * Returns exit status or 0 for background processes.
 */
int execute_command(const Command *command);

/*
 * Spawn one external command child with specified process group pgid.
 * Pass pgid=0 to make the new child its own process group leader.
 * Returns child pid in parent, or -1 on failure.
 */
pid_t spawn_child(const Command *command, int input_fd, int output_fd,
                  int close_fd, pid_t pgid);

/*
 * Synchronize with a child process by PID and decode its exit status.
 */
int wait_for_child(pid_t pid);

#endif
