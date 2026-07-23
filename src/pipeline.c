#define _POSIX_C_SOURCE 200809L

#include "pipeline.h"


#include "executor.h"
#include "jobs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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

int execute_pipeline(const Command *commands)
{
    const Command *command;
    pid_t *pids;
    size_t command_count;
    size_t child_count = 0;
    int previous_read_fd = -1;
    int execution_failed = 0;
    pid_t leader_pgid = 0;

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
                                has_next ? pipe_fds[1] : -1, pipe_fds[0], leader_pgid);
        if (child_pid < 0) {
            close_if_open(previous_read_fd);
            previous_read_fd = -1;
            close_if_open(pipe_fds[0]);
            close_if_open(pipe_fds[1]);
            execution_failed = 1;
            break;
        }

        if (leader_pgid == 0) {
            leader_pgid = child_pid;
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

    if (execution_failed) {
        free(pids);
        return 1;
    }

    if (child_count > 0) {
        char *cmd_str = format_command_string(commands);
        pid_t last_pid = pids[child_count - 1];
        Job *job = add_job(last_pid, leader_pgid, cmd_str, commands->run_in_background);
        free(cmd_str);

        if (commands->run_in_background) {
            if (job != NULL) {
                printf("[%d] %d\n", job->job_id, (int)last_pid);
                fflush(stdout);
            }
            free(pids);
            return 0;
        }

        int final_status = wait_for_job(job);
        free(pids);
        return final_status;
    }

    free(pids);
    return 1;
}
