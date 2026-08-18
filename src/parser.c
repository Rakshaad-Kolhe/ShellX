#include "parser.h"
#include "lexer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void free_command_list(Command *commands)
{
    while (commands != NULL) {
        Command *next = commands->next;
        size_t index;

        for (index = 0; index < commands->arg_count; index++) {
            free(commands->args[index]);
            commands->args[index] = NULL;
        }

        free(commands->input_path);
        commands->input_path = NULL;
        free(commands->output_path);
        commands->output_path = NULL;
        free(commands);

        commands = next;
    }
}

Command *parse_command_line_with_context(const char *input,
                                   const ShellContext *context)
{
    TokenList tokens;
    Command *head = NULL;
    Command *command = NULL;

    if (input == NULL) {
        return NULL;
    }

    if (!tokenize(input, context, &tokens)) {
        return NULL;
    }

    if (tokens.count == 0) {
        free_token_list(&tokens);
        return NULL;
    }

    head = allocate_command();
    if (head == NULL) {
        free_token_list(&tokens);
        return NULL;
    }
    command = head;

    for (size_t i = 0; i < tokens.count; i++) {
        Token *tok = &tokens.tokens[i];

        if (tok->type == TOKEN_PIPE) {
            Command *next_command;

            if (command->arg_count == 0) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            if (i + 1 >= tokens.count || tokens.tokens[i + 1].type == TOKEN_PIPE) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            next_command = allocate_command();
            if (next_command == NULL) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            command->next = next_command;
            command = next_command;
            continue;
        }

        if (tok->type == TOKEN_REDIRECT_IN) {
           if (i + 1 >= tokens.count || tokens.tokens[i + 1].type != TOKEN_WORD) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            if (command->input_path != NULL) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            i++;
            command->input_path = tokens.tokens[i].value;
            tokens.tokens[i].value = NULL;
            continue;
        }

        if (tok->type == TOKEN_REDIRECT_OUT || tok->type == TOKEN_REDIRECT_APPEND) {
            int append_output = (tok->type == TOKEN_REDIRECT_APPEND);

            if (i + 1 >= tokens.count || tokens.tokens[i + 1].type != TOKEN_WORD) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            if (command->output_path != NULL) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            i++;
            command->output_path = tokens.tokens[i].value;
            tokens.tokens[i].value = NULL;
            command->append_output = append_output;
            continue;
        }

        if (tok->type == TOKEN_BACKGROUND) {
            if (command->arg_count == 0 || i + 1 < tokens.count) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            head->run_in_background = 1;
            break;
        }

        if (tok->type == TOKEN_WORD) {
            if (command->arg_count >= SHELLX_MAX_ARGS - 1) {
                free_command_list(head);
                free_token_list(&tokens);
                return NULL;
            }

            command->args[command->arg_count] = tok->value;
            tok->value = NULL;
            command->arg_count++;
            command->args[command->arg_count] = NULL;
            continue;
        }
    }

    Command *curr = head;
    while (curr != NULL) {
        if (curr->arg_count == 0) {
            free_command_list(head);
            free_token_list(&tokens);
            return NULL;
        }
        curr = curr->next;
    }

    free_token_list(&tokens);
    return head;
}

Command *parse_command_line(const char *input)
{
    ShellContext default_context = {
        .last_exit_status = 0,
        .shell_pid = getpid()
    };

    return parse_command_line_with_context(input, &default_context);
}
