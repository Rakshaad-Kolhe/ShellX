#ifndef SHELLX_PARSER_H
#define SHELLX_PARSER_H

#include "pipeline.h"

/* Parse one input line into a linked list of Command structures. */
Command *parse_command_line(const char *input);

/* Free a Command list returned by parse_command_line. */
void free_command_list(Command *commands);

#endif
