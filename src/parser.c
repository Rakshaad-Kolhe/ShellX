#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void initialize_command(Command *command)
{
    size_t index;

    for (index = 0; index < SHELLX_MAX_ARGS; index++) {
        command->args[index] = NULL;
    }

    command->arg_count = 0;
    command->input_path = NULL;
    command->output_path = NULL;
    command->append_output = 0;
    command->run_in_background = 0;
    command->next = NULL;
}

static Command *allocate_command(void)
{
    Command *command = malloc(sizeof(*command));

    if (command == NULL) {
        return NULL;
    }

    initialize_command(command);
    return command;
}

static char *copy_token(const char *start, size_t length)
{
    char *copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, start, length);
    copy[length] = '\0';

    return copy;
}

static void skip_whitespace(const char **cursor)
{
    while (**cursor != '\0' && isspace((unsigned char)**cursor)) {
        (*cursor)++;
    }
}

static int is_operator(char character)
{
    return character == '|' || character == '<' ||
           character == '>' || character == '&';
}

static char *parse_word(const char **cursor)
{
    const char *start = *cursor;

    while (**cursor != '\0' &&
           !isspace((unsigned char)**cursor) &&
           !is_operator(**cursor)) {
        (*cursor)++;
    }

    if (*cursor == start) {
        return NULL;
    }

    return copy_token(start, (size_t)(*cursor - start));
}

static int append_argument(Command *command, char *argument)
{
    if (command->arg_count >= SHELLX_MAX_ARGS - 1) {
        free(argument);
        return 0;
    }

    command->args[command->arg_count] = argument;
    command->arg_count++;
    command->args[command->arg_count] = NULL;

    return 1;
}

static int parse_redirection(Command *command, const char **cursor)
{
    char operator = **cursor;
    int append_output = 0;
    char *path;

    (*cursor)++;
    if (operator == '>' && **cursor == '>') {
        append_output = 1;
        (*cursor)++;
    }

    skip_whitespace(cursor);
    if (**cursor == '\0' || is_operator(**cursor)) {
        return 0;
    }

    path = parse_word(cursor);
    if (path == NULL) {
        return 0;
    }

    if (operator == '<') {
        if (command->input_path != NULL) {
            free(path);
            return 0;
        }
        command->input_path = path;
        return 1;
    }

    if (command->output_path != NULL) {
        free(path);
        return 0;
    }

    command->output_path = path;
    command->append_output = append_output;
    return 1;
}

static int remaining_input_is_whitespace(const char *cursor)
{
    skip_whitespace(&cursor);
    return *cursor == '\0';
}

Command *parse_command_line(const char *input)
{
    Command *head;
    Command *command;
    const char *cursor;

    if (input == NULL) {
        return NULL;
    }

    cursor = input;
    skip_whitespace(&cursor);

    if (*cursor == '\0') {
        return NULL;
    }

    head = allocate_command();
    if (head == NULL) {
        return NULL;
    }

    command = head;

    while (*cursor != '\0') {
        char *argument;

        skip_whitespace(&cursor);
        if (*cursor == '\0') {
            break;
        }

        if (*cursor == '|') {
            Command *next_command;

            if (command->arg_count == 0) {
                free_command_list(head);
                return NULL;
            }

            cursor++;
            skip_whitespace(&cursor);
            if (*cursor == '\0' || *cursor == '|') {
                free_command_list(head);
                return NULL;
            }

            next_command = allocate_command();
            if (next_command == NULL) {
                free_command_list(head);
                return NULL;
            }

            command->next = next_command;
            command = next_command;
            continue;
        }

        if (*cursor == '<' || *cursor == '>') {
            if (!parse_redirection(command, &cursor)) {
                free_command_list(head);
                return NULL;
            }
            continue;
        }

        if (*cursor == '&') {
            cursor++;
            if (command->arg_count == 0 || !remaining_input_is_whitespace(cursor)) {
                free_command_list(head);
                return NULL;
            }

            head->run_in_background = 1;
            break;
        }

        argument = parse_word(&cursor);
        if (argument == NULL || !append_argument(command, argument)) {
            free_command_list(head);
            return NULL;
        }
    }

    if (command->arg_count == 0) {
        free_command_list(head);
        return NULL;
    }

    return head;
}

void free_command_list(Command *commands)
{
    while (commands != NULL) {
        Command *next = commands->next;
        size_t index;

        for (index = 0; index < commands->arg_count; index++) {
            free(commands->args[index]);
        }

        free(commands->input_path);
        free(commands->output_path);
        free(commands);

        commands = next;
    }
}
