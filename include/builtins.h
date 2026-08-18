#ifndef SHELLX_BUILTINS_H
#define SHELLX_BUILTINS_H

#include "pipeline.h"

/*
 * Return nonzero when the borrowed command names a supported built-in:
 * cd, exit, jobs, fg, bg, export, unset, env.
 * The command is not modified or freed.
 */
int is_builtin(const Command *command);

/*
 * Execute a borrowed built-in command in the current ShellX process.
 * Stores nonzero in *should_exit when the shell should terminate.
 * Returns zero on success and nonzero on failure.
 */
int execute_builtin(const Command *command, int *should_exit);

#endif
