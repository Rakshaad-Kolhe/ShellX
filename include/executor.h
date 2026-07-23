#ifndef SHELLX_EXECUTOR_H
#define SHELLX_EXECUTOR_H

#include "pipeline.h"

#include <sys/types.h>

/*
 * Execute one foreground command.
 * The command is borrowed and must not be modified or freed.
 * Returns the command's exit status when available, or nonzero on failure.
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

#endif
