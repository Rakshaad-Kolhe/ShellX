#ifndef SHELLX_JOBS_H
#define SHELLX_JOBS_H

#include "pipeline.h"

#include <sys/types.h>

typedef enum JobState {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct Job {
    int job_id;               /* 1-based sequential job ID */
    pid_t pgid;               /* Process Group ID (leader PID) */
    pid_t pid;                /* Leader process PID */
    char *command;            /* Reconstructed command string */
    JobState state;           /* JobState: JOB_RUNNING, JOB_STOPPED, JOB_DONE */
    int is_background;        /* 1 if background job, 0 if foreground */
    struct Job *next;         /* Pointer to next job in internal list */
} Job;

/*
 * Initialize or reset the internal job table.
 */
void init_job_table(void);

/*
 * Create and register a new job in the internal job table.
 * Returns a pointer to the created Job struct, or NULL on allocation failure.
 */
Job *add_job(pid_t pid, pid_t pgid, const char *command_str, int is_background);

/*
 * Find a registered job by its 1-based job_id.
 * Returns pointer to Job if found, or NULL if not found.
 */
Job *find_job_by_id(int job_id);

/*
 * Find a registered job by process PID or process group PGID.
 * Returns pointer to Job if found, or NULL if not found.
 */
Job *find_job_by_pid(pid_t pid);

/*
 * Remove a job from the job table by job_id and free its resources.
 * Returns 1 if removed, 0 if job was not found.
 */
int remove_job_by_id(int job_id);

/*
 * Remove all completed (JOB_DONE) jobs from the internal job table.
 * Returns number of jobs removed.
 */
int remove_completed_jobs(void);

/*
 * Destroy all registered jobs and free memory allocated by the job table.
 */
void destroy_job_table(void);

/*
 * Format a readable command line string from a borrowed Command AST list.
 * Caller must free the returned string.
 */
char *format_command_string(const Command *commands);

#endif
