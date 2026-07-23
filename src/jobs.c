#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
