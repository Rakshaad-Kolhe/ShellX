#include "../include/jobs.h"
#include "../include/pipeline.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_init_and_add_job(void)
{
    Job *job;

    init_job_table();

    job = add_job(1001, 1001, "sleep 5 &", 1);
    assert(job != NULL);
    assert(job->job_id == 1);
    assert(job->pid == 1001);
    assert(job->pgid == 1001);
    assert(strcmp(job->command, "sleep 5 &") == 0);
    assert(job->state == JOB_RUNNING);
    assert(job->is_background == 1);

    destroy_job_table();
}

static void test_unique_sequential_job_ids(void)
{
    Job *job1;
    Job *job2;
    Job *job3;

    init_job_table();

    job1 = add_job(2001, 2001, "echo first &", 1);
    job2 = add_job(2002, 2002, "echo second &", 1);
    job3 = add_job(2003, 2003, "echo third &", 1);

    assert(job1 != NULL && job1->job_id == 1);
    assert(job2 != NULL && job2->job_id == 2);
    assert(job3 != NULL && job3->job_id == 3);

    destroy_job_table();
}

static void test_find_job_by_id_and_pid(void)
{
    Job *job;

    init_job_table();

    add_job(3001, 3000, "cat file &", 1);
    add_job(3002, 3002, "sleep 10 &", 1);

    job = find_job_by_id(1);
    assert(job != NULL);
    assert(job->pid == 3001);

    job = find_job_by_pid(3002);
    assert(job != NULL);
    assert(job->job_id == 2);

    job = find_job_by_pid(3000);
    assert(job != NULL && job->job_id == 1);

    assert(find_job_by_id(999) == NULL);
    assert(find_job_by_pid(9999) == NULL);

    destroy_job_table();
}

static void test_remove_job_by_id(void)
{
    init_job_table();

    add_job(4001, 4001, "sleep 1 &", 1);
    add_job(4002, 4002, "sleep 2 &", 1);

    assert(find_job_by_id(1) != NULL);
    assert(remove_job_by_id(1) == 1);
    assert(find_job_by_id(1) == NULL);
    assert(find_job_by_id(2) != NULL);

    assert(remove_job_by_id(999) == 0);

    destroy_job_table();
}

static void test_remove_completed_jobs(void)
{
    Job *job1;
    Job *job2;

    init_job_table();

    job1 = add_job(5001, 5001, "finished job", 1);
    job2 = add_job(5002, 5002, "running job", 1);

    assert(job1 != NULL && job2 != NULL);
    job1->state = JOB_DONE;

    assert(remove_completed_jobs() == 1);
    assert(find_job_by_id(1) == NULL);
    assert(find_job_by_id(2) != NULL);

    destroy_job_table();
}

static void test_format_command_string(void)
{
    Command first = {0};
    Command second = {0};
    char *formatted;

    first.args[0] = "cat";
    first.arg_count = 1;
    first.input_path = "input.txt";
    first.next = &second;
    first.run_in_background = 1;

    second.args[0] = "grep";
    second.args[1] = "hello";
    second.arg_count = 2;
    second.output_path = "output.txt";
    second.append_output = 0;
    second.next = NULL;

    formatted = format_command_string(&first);
    assert(formatted != NULL);
    assert(strcmp(formatted, "cat < input.txt | grep hello > output.txt &") == 0);
    free(formatted);
}

int main(void)
{
    test_init_and_add_job();
    test_unique_sequential_job_ids();
    test_find_job_by_id_and_pid();
    test_remove_job_by_id();
    test_remove_completed_jobs();
    test_format_command_string();

    printf("All job table tests passed.\n");
    return 0;
}
