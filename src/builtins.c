#define _POSIX_C_SOURCE 200809L

#include "builtins.h"


#include "jobs.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static int execute_cd(const Command *command)
{
    const char *directory;

    if (command->arg_count > 2) {
        fprintf(stderr, "shellx: cd: too many arguments\n");
        return 1;
    }

    if (command->arg_count == 1) {
        directory = getenv("HOME");
        if (directory == NULL || directory[0] == '\0') {
            fprintf(stderr, "shellx: cd: HOME not set\n");
            return 1;
        }
    } else {
        directory = command->args[1];
    }

    if (chdir(directory) != 0) {
        perror("shellx: cd");
        return 1;
    }

    return 0;
}

static int execute_exit(const Command *command, int *should_exit)
{
    if (command->arg_count > 1) {
        fprintf(stderr, "shellx: exit: arguments are not supported\n");
        return 1;
    }

    *should_exit = 1;
    return 0;
}

static int parse_job_specifier(const char *arg)
{
    if (arg == NULL) {
        return 1;
    }

    if (arg[0] == '%') {
        return atoi(arg + 1);
    }

    return atoi(arg);
}

static int execute_jobs(void)
{
    print_jobs();
    return 0;
}

static int execute_fg(const Command *command)
{
    int job_id = 1;
    Job *job;

    if (command->arg_count > 1) {
        job_id = parse_job_specifier(command->args[1]);
    }

    if (job_id <= 0) {
        fprintf(stderr, "shellx: fg: invalid job specifier\n");
        return 1;
    }

    job = find_job_by_id(job_id);
    if (job == NULL) {
        fprintf(stderr, "shellx: fg: %s: no such job\n", command->arg_count > 1 ? command->args[1] : "1");
        return 1;
    }

    printf("%s\n", job->command);
    fflush(stdout);

    if (job->state == JOB_STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("shellx: fg (SIGCONT)");
        }
        job->state = JOB_RUNNING;
    }

    return wait_for_job(job);
}

static int execute_bg(const Command *command)
{
    int job_id = 1;
    Job *job;

    if (command->arg_count > 1) {
        job_id = parse_job_specifier(command->args[1]);
    }

    if (job_id <= 0) {
        fprintf(stderr, "shellx: bg: invalid job specifier\n");
        return 1;
    }

    job = find_job_by_id(job_id);
    if (job == NULL) {
        fprintf(stderr, "shellx: bg: %s: no such job\n", command->arg_count > 1 ? command->args[1] : "1");
        return 1;
    }

    if (job->state == JOB_STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("shellx: bg (SIGCONT)");
        }
        job->state = JOB_RUNNING;
    }

    printf("[%d]+ %s &\n", job->job_id, job->command);
    fflush(stdout);

    return 0;
}

int is_builtin(const Command *command)
{
    if (command == NULL || command->arg_count == 0 || command->args[0] == NULL) {
        return 0;
    }

    return strcmp(command->args[0], "cd") == 0 ||
           strcmp(command->args[0], "exit") == 0 ||
           strcmp(command->args[0], "jobs") == 0 ||
           strcmp(command->args[0], "fg") == 0 ||
           strcmp(command->args[0], "bg") == 0;
}

int execute_builtin(const Command *command, int *should_exit)
{
    if (command == NULL || command->arg_count == 0 ||
        command->args[0] == NULL || should_exit == NULL) {
        return 1;
    }

    *should_exit = 0;

    if (strcmp(command->args[0], "cd") == 0) {
        return execute_cd(command);
    }

    if (strcmp(command->args[0], "exit") == 0) {
        return execute_exit(command, should_exit);
    }

    if (strcmp(command->args[0], "jobs") == 0) {
        return execute_jobs();
    }

    if (strcmp(command->args[0], "fg") == 0) {
        return execute_fg(command);
    }

    if (strcmp(command->args[0], "bg") == 0) {
        return execute_bg(command);
    }

    return 1;
}
