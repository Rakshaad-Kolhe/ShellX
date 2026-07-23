#ifndef SHELLX_PIPELINE_H
#define SHELLX_PIPELINE_H

#include <stddef.h>

#define SHELLX_MAX_ARGS 128

typedef struct Command {
    char *args[SHELLX_MAX_ARGS];
    size_t arg_count;
    char *input_path;
    char *output_path;
    int append_output;
    int run_in_background;
    struct Command *next;
} Command;

/*
 * Execute a borrowed linked list of Command structures as one foreground pipeline.
 * The command list must not be modified or freed by this function.
 * Returns the final pipeline command's exit status when available, or a nonzero
 * value on execution failure.
 */
int execute_pipeline(const Command *commands);

#endif
