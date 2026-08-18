#define _POSIX_C_SOURCE 200809L

#include "executor.h"
#include "builtins.h"
#include "signals.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void close_if_open(int fd)
{
    if (fd != -1) {
        close(fd);
    }
}

static int redirect_input(const char *path)
{
    int input_fd;

    input_fd = open(path, O_RDONLY);
    if (input_fd < 0) {
        perror(path);
        return 1;
    }

    if (dup2(input_fd, STDIN_FILENO) < 0) {
        perror("dup2");
        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }
        return 1;
    }

    if (input_fd != STDIN_FILENO) {
        close(input_fd);
    }

    return 0;
}

static int redirect_output(const char *path, int append)
{
    int flags;
    int output_fd;

    flags = O_WRONLY | O_CREAT;
    if (append) {
        flags |= O_APPEND;
    } else {
        flags |= O_TRUNC;
    }

    output_fd = open(path, flags, 0666);
    if (output_fd < 0) {
        perror(path);
        return 1;
    }

    if (dup2(output_fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        if (output_fd != STDOUT_FILENO) {
            close(output_fd);
        }
        return 1;
    }

    if (output_fd != STDOUT_FILENO) {
        close(output_fd);
    }

    return 0;
}

static int apply_redirections(const Command *command)
{
    if (command->input_path != NULL && redirect_input(command->input_path) != 0) {
        return 1;
    }

    if (command->output_path != NULL &&
        redirect_output(command->output_path, command->append_output) != 0) {
        return 1;
    }

    return 0;
}

static int setup_child_stdio(int input_fd, int output_fd)
{
    if (input_fd != -1 && dup2(input_fd, STDIN_FILENO) == -1) {
        perror("dup2");
        return 1;
    }

    if (output_fd != -1 && dup2(output_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        return 1;
    }

    return 0;
}

pid_t spawn_child(const Command *command, int input_fd, int output_fd,
                  int close_fd, pid_t pgid)
{
    pid_t child_pid;

    if (command == NULL || command->arg_count == 0 || command->args[0] == NULL) {
        return -1;
    }

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return -1;
    }

    if (child_pid == 0) {
        pid_t my_pid = getpid();
        pid_t target_pgid = (pgid == 0) ? my_pid : pgid;

        setpgid(0, target_pgid);
        setup_child_signals();

        if (setup_child_stdio(input_fd, output_fd) != 0) {
            close_if_open(input_fd);
            close_if_open(output_fd);
            close_if_open(close_fd);
            _exit(126);
        }

        close_if_open(input_fd);
        close_if_open(output_fd);
        close_if_open(close_fd);

        if (apply_redirections(command) != 0) {
            _exit(1);
        }

        if (is_builtin(command)) {
            int should_exit = 0;
            int status = execute_builtin(command, &should_exit);
            _exit(status);
        }

        execvp(command->args[0], command->args);
        perror("execvp");
        _exit(127);
    }

    pid_t target_pgid = (pgid == 0) ? child_pid : pgid;
    setpgid(child_pid, target_pgid);

    return child_pid;
}

int wait_for_child(pid_t pid)
{
    int status;

    if (pid <= 0) {
        return 1;
    }

    while (waitpid(pid, &status, WUNTRACED) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

int execute_command(const Command *command)
{
    pid_t child_pid;
    char *cmd_str;
    Job *job;

    if (command == NULL || command->arg_count == 0 || command->args[0] == NULL) {
        return 1;
    }

    child_pid = spawn_child(command, -1, -1, -1, 0);
    if (child_pid < 0) {
        return 1;
    }

    cmd_str = format_command_string(command);
    job = add_job(child_pid, child_pid, cmd_str, command->run_in_background);
    free(cmd_str);

    if (command->run_in_background) {
        if (job != NULL) {
            printf("[%d] %d\n", job->job_id, (int)child_pid);
            fflush(stdout);
        }
        return 0;
    }

    return wait_for_job(job);
}
