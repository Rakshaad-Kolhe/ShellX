#include "builtins.h"
#include "executor.h"
#include "parser.h"

#include "pipeline.h"

#include <stdio.h>

int main(void)
{
    char line[1024];
    int should_exit = 0;

    while (!should_exit) {
        Command *commands;

        printf("shellx> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        commands = parse_command_line(line);
        if (commands == NULL) {
            continue;
        }

        if (commands->next != NULL) {
            execute_pipeline(commands);
        } else if (is_builtin(commands)) {
            execute_builtin(commands, &should_exit);
        } else {
            execute_command(commands);
        }

        free_command_list(commands);
    }

    return 0;
}
