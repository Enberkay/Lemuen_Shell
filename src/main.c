// lemuen/src/main.c - Lemuen Shell v0.4 (restructured)
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 64
#define MAX_CMDS 8

void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\nlemuen> ", 9);
    fflush(stdout);
}

int parse_command(char *input, char **argv, char **infile, char **outfile, int *append) {
    int argc = 0;
    *infile = NULL;
    *outfile = NULL;
    *append = 0;

    char *token = strtok(input, " ");
    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token) *infile = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token) *outfile = token;
            *append = 0;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " ");
            if (token) *outfile = token;
            *append = 1;
        } else {
            argv[argc++] = token;
        }
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    return argc;
}

int is_builtin(char **argv) {
    return (strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "exit") == 0);
}

int run_builtin(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        const char *path = argv[1] ? argv[1] : getenv("HOME");
        if (chdir(path) != 0)
            perror("lemuen: cd");
        return 1;
    }
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    return 0;
}

void execute_pipeline(char *line) {
    char *cmds[MAX_CMDS];
    int n_cmds = 0;

    char *segment = strtok(line, "|");
    while (segment && n_cmds < MAX_CMDS) {
        cmds[n_cmds++] = segment;
        segment = strtok(NULL, "|");
    }

    int in_fd = 0, pipefd[2];

    for (int i = 0; i < n_cmds; ++i) {
        char *argv[MAX_ARGS];
        char *infile = NULL, *outfile = NULL;
        int append = 0;

        parse_command(cmds[i], argv, &infile, &outfile, &append);
        if (!argv[0]) continue;

        if (is_builtin(argv) && n_cmds == 1) {
            run_builtin(argv);
            return;
        }

        if (i < n_cmds - 1) pipe(pipefd);

        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < n_cmds - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            if (infile) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) { perror("lemuen: open input"); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (outfile) {
                int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                int fd = open(outfile, flags, 0644);
                if (fd < 0) { perror("lemuen: open output"); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(argv[0], argv);
            perror("lemuen");
            exit(1);
        } else {
            wait(NULL);
            if (i > 0) close(in_fd);
            if (i < n_cmds - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        }
    }
}

int main() {
    signal(SIGINT, handle_sigint);
    using_history();

    char *cmd;
    while ((cmd = readline("lemuen> ")) != NULL) {
        if (*cmd) {
            add_history(cmd);
            execute_pipeline(cmd);
        }
        free(cmd);
    }

    printf("\n");
    return 0;
}