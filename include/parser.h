#ifndef SHELLX_PARSER_H
#define SHELLX_PARSER_H

#include "expansion.h"
#include "pipeline.h"

/*
+ Parse one input line into a linked list of Command structures using
+ an explicit ShellContext for variable/parameter expansion ($?, $$[]).
+*/
Command *parse_command_line_with_context(const char *input,
                                   const ShellContext *context);

/*
+ Parse one input line using default shell context ($? = 0, $$ = getpid()).
+ Retained for backwards compatibility.
+*/
Command *parse_command_line(const char *input);

/* Free a Command list returned by parse_command_line / parse_command_line_with_context. */
void free_command_list(Command *commands);

#endif
