#define _POSIX_C_SOURCE 200809L

#include "jobs.h"


#include "signals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static Job *job_list_head = NULL;
static int next_job_id_counter = 1;

static char *duplicate_string(const char *str)
{
    size_t length;
    char *copy;

    if (str == NULL) {
        str = "";
    }

    length = strlen(str);
    copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, length + 1);
    return copy;
}

void init_job_table(void)
{
    destroy_job_table();
    job_list_head = NULL;
    next_job_id_counter = 1;
}

Job *add_job(pid_t pid, pid_t pgid, const char *command_str, int is_background)
{
    Job *job;
    Job *cursor;

    job = malloc(sizeof(*job));
    if (job == NULL) {
        return NULL;
    }

    job->job_id = next_job_id_counter++;
    job->pid = pid;
    job->pgid = (pgid > 0) ? pgid : pid;
    job->command = duplicate_string(command_str);
    if (job->command == NULL) {
        free(job);
        return NULL;
    }

    job->state = JOB_RUNNING;
    job->is_background = is_background;
    job->next = NULL;

    if (job_list_head == NULL) {
        job_list_head = job;
    } else {
        cursor = job_list_head;
        while (cursor->next != NULL) {
            cursor = cursor->next;
        }
        cursor->next = job;
    }

    return job;
}

Job *find_job_by_id(int job_id)
{
    Job *cursor = job_list_head;

    while (cursor != NULL) {
        if (cursor->job_id == job_id) {
            return cursor;
        }
        cursor = cursor->next;
    }

    return NULL;
}

Job *find_job_by_pid(pid_t pid)
{
    Job *cursor = job_list_head;

    while (cursor != NULL) {
        if (cursor->pid == pid || cursor->pgid == pid) {
            return cursor;
        }
        cursor = cursor->next;
    }

    return NULL;
}

int remove_job_by_id(int job_id)
{
    Job *prev = NULL;
    Job *curr = job_list_head;

    while (curr != NULL) {
        if (curr->job_id == job_id) {
            if (prev == NULL) {
                job_list_head = curr->next;
            } else {
                prev->next = curr->next;
            }

            free(curr->command);
            free(curr);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }

    return 0;
}

int remove_completed_jobs(void)
{
    Job *prev = NULL;
    Job *curr = job_list_head;
    int count = 0;

    while (curr != NULL) {
        Job *next = curr->next;

        if (curr->state == JOB_DONE) {
            if (prev == NULL) {
                job_list_head = next;
            } else {
                prev->next = next;
            }

            free(curr->command);
            free(curr);
            count++;
            curr = next;
        } else {
            prev = curr;
            curr = next;
        }
    }

    return count;
}

void update_job_status(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        Job *job = find_job_by_pid(pid);
        if (job != NULL) {
            if (WIFSTOPPED(status)) {
                job->state = JOB_STOPPED;
            } else if (WIFCONTINUED(status)) {
                job->state = JOB_RUNNING;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->state = JOB_DONE;
            }
        }
    }
}

void print_jobs(void)
{
    Job *cursor;

    update_job_status();

    cursor = job_list_head;
    while (cursor != NULL) {
        const char *state_str = "Running";
        char current_marker = ' ';

        if (cursor->state == JOB_STOPPED) {
            state_str = "Stopped";
            current_marker = '+';
        } else if (cursor->state == JOB_DONE) {
            state_str = "Done";
        }

        printf("[%d]%c %-24s %s\n", cursor->job_id, current_marker, state_str, cursor->command);
        cursor = cursor->next;
    }

    remove_completed_jobs();
}

int wait_for_job(Job *job)
{
    int status = 0;
    pid_t pid;

    if (job == NULL) {
        return 1;
    }

    give_terminal_to(job->pgid);

    while ((pid = waitpid(-job->pgid, &status, WUNTRACED)) > 0) {
        if (WIFSTOPPED(status)) {
            job->state = JOB_STOPPED;
            printf("\n[%d]+ Stopped                 %s\n", job->job_id, job->command);
            fflush(stdout);
            break;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            job->state = JOB_DONE;
        }
    }

    give_terminal_to(get_shell_pgid());

    if (job->state == JOB_DONE) {
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        remove_job_by_id(job->job_id);
        return exit_code;
    }

    return 128 + WSTOPSIG(status);
}

void destroy_job_table(void)
{
    Job *curr = job_list_head;

    while (curr != NULL) {
        Job *next = curr->next;

        free(curr->command);
        free(curr);
        curr = next;
    }

    job_list_head = NULL;
    next_job_id_counter = 1;
}

char *format_command_string(const Command *commands)
{
    const Command *cmd;
    size_t total_length = 0;
    char *buffer;
    size_t offset = 0;

    if (commands == NULL) {
        return duplicate_string("");
    }

    cmd = commands;
    while (cmd != NULL) {
        size_t index;

        for (index = 0; index < cmd->arg_count; index++) {
            if (cmd->args[index] != NULL) {
                total_length += strlen(cmd->args[index]) + 1;
            }
        }

        if (cmd->input_path != NULL) {
            total_length += strlen(cmd->input_path) + 4;
        }
        if (cmd->output_path != NULL) {
            total_length += strlen(cmd->output_path) + 5;
        }
        if (cmd->next != NULL) {
            total_length += 3;
        }

        cmd = cmd->next;
    }

    if (commands->run_in_background) {
        total_length += 3;
    }

    total_length += 1;

    buffer = malloc(total_length);
    if (buffer == NULL) {
        return duplicate_string("");
    }

    buffer[0] = '\0';
    cmd = commands;

    while (cmd != NULL) {
        size_t index;

        for (index = 0; index < cmd->arg_count; index++) {
            if (cmd->args[index] != NULL) {
                if (offset > 0 && buffer[offset - 1] != ' ') {
                    offset += snprintf(buffer + offset, total_length - offset, " ");
                }
                offset += snprintf(buffer + offset, total_length - offset, "%s", cmd->args[index]);
            }
        }

        if (cmd->input_path != NULL) {
            offset += snprintf(buffer + offset, total_length - offset, " < %s", cmd->input_path);
        }

        if (cmd->output_path != NULL) {
            offset += snprintf(buffer + offset, total_length - offset, " %s %s",
                               cmd->append_output ? ">>" : ">", cmd->output_path);
        }

        if (cmd->next != NULL) {
            offset += snprintf(buffer + offset, total_length - offset, " |");
        }

        cmd = cmd->next;
    }

    if (commands->run_in_background) {
        offset += snprintf(buffer + offset, total_length - offset, " &");
    }

    return buffer;
}
