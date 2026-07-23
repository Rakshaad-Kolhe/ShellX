#include "executor.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
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
                  int close_fd)
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

        execvp(command->args[0], command->args);
        perror("execvp");
        _exit(127);
    }

    return child_pid;
}

int wait_for_child(pid_t pid)
{
    int status;

    if (pid <= 0) {
        return 1;
    }

    while (waitpid(pid, &status, 0) == -1) {
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

    if (command == NULL || command->arg_count == 0 || command->args[0] == NULL) {
        return 1;
    }

    child_pid = spawn_child(command, -1, -1, -1);
    if (child_pid < 0) {
        return 1;
    }

    return wait_for_child(child_pid);
}
