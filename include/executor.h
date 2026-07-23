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
 * Returns the command's exit status when available, or zero for background processes.
 */
int execute_command(const Command *command);

/*
 * Spawn one external command child with optional inherited stdin/stdout fds.
 * Pass -1 for either fd to leave that stream unchanged before redirections.
 * The command is borrowed and must not be modified or freed.
 * Returns the child pid in the parent, or -1 when fork fails.
 */
pid_t spawn_child(const Command *command, int input_fd, int output_fd,
                  int close_fd);

/*
 * Synchronize with a child process by PID and decode its exit status.
 * Handles EINTR signal interruptions and returns exit status (0-255, 127 on error,
 * or 128 + signal on termination).
 */
int wait_for_child(pid_t pid);

#endif
