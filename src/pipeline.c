#include "pipeline.h"

#include "executor.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int validate_commands(const Command *commands)
{
    const Command *command = commands;

    if (command == NULL) {
        return 0;
    }

    while (command != NULL) {
        if (command->arg_count == 0 || command->args[0] == NULL) {
            return 0;
        }

        command = command->next;
    }

    return 1;
}

static size_t count_commands(const Command *commands)
{
    size_t count = 0;
    const Command *command = commands;

    while (command != NULL) {
        count++;
        command = command->next;
    }

    return count;
}

static int wait_for_child(pid_t pid, int *status)
{
    while (waitpid(pid, status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 0;
        }
    }

    return 1;
}

static int decode_wait_status(int status)
{
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

static void close_if_open(int fd)
{
    if (fd != -1) {
        close(fd);
    }
}

int execute_pipeline(const Command *commands)
{
    const Command *command;
    pid_t *pids;
    size_t command_count;
    size_t child_count = 0;
    int previous_read_fd = -1;
    int execution_failed = 0;
    int final_status = 1;

    if (!validate_commands(commands)) {
        return 1;
    }

    command_count = count_commands(commands);
    pids = malloc(command_count * sizeof(*pids));
    if (pids == NULL) {
        return 1;
    }

    command = commands;
    while (command != NULL) {
        int pipe_fds[2] = {-1, -1};
        int has_next = command->next != NULL;
        pid_t child_pid;

        if (has_next && pipe(pipe_fds) == -1) {
            perror("pipe");
            close_if_open(previous_read_fd);
            previous_read_fd = -1;
            execution_failed = 1;
            break;
        }

        child_pid = spawn_child(command, previous_read_fd,
                                has_next ? pipe_fds[1] : -1, pipe_fds[0]);
        if (child_pid < 0) {
            close_if_open(previous_read_fd);
            previous_read_fd = -1;
            close_if_open(pipe_fds[0]);
            close_if_open(pipe_fds[1]);
            execution_failed = 1;
            break;
        }

        pids[child_count] = child_pid;
        child_count++;

        close_if_open(previous_read_fd);
        previous_read_fd = -1;

        if (has_next) {
            close_if_open(pipe_fds[1]);
            previous_read_fd = pipe_fds[0];
        }

        command = command->next;
    }

    close_if_open(previous_read_fd);

    if (child_count > 0) {
        size_t index;

        for (index = 0; index < child_count; index++) {
            int status;

            if (!wait_for_child(pids[index], &status)) {
                execution_failed = 1;
                continue;
            }

            if (index == child_count - 1) {
                final_status = decode_wait_status(status);
            }
        }
    }

    if (execution_failed) {
        if (child_count == 0) {
            free(pids);
            return 1;
        }

        free(pids);
        return 1;
    }

    free(pids);
    return final_status;
}
