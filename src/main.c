// lemuen/src/main.c - Lemuen Shell v0.3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define MAX_CMDS 8

void handle_sigint(int sig) {
    (void)sig;
    printf("\nlemuen> ");
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
    return strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "exit") == 0;
}

int run_builtin(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) argv[1] = getenv("HOME");
        if (chdir(argv[1]) != 0) perror("lemuen: cd");
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

    int in_fd = 0;
    int pipefd[2];

    for (int i = 0; i < n_cmds; i++) {
        char *argv[MAX_ARGS];
        char *infile = NULL, *outfile = NULL;
        int append = 0;
        parse_command(cmds[i], argv, &infile, &outfile, &append);

        if (argv[0] == NULL) continue;
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
                if (fd < 0) perror("lemuen input"), exit(1);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (outfile) {
                int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                int fd = open(outfile, flags, 0644);
                if (fd < 0) perror("lemuen output"), exit(1);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(argv[0], argv);
            perror("lemuen");
            exit(1);
        } else if (pid > 0) {
            wait(NULL);
            if (i > 0) close(in_fd);
            if (i < n_cmds - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        } else {
            perror("fork");
        }
    }
}

int main() {
    char cmd[MAX_CMD_LEN];
    signal(SIGINT, handle_sigint);

    while (1) {
        printf("lemuen> ");
        fflush(stdout);

        if (!fgets(cmd, MAX_CMD_LEN, stdin)) {
            printf("\n");
            break;
        }

        cmd[strcspn(cmd, "\n")] = '\0';
        if (strlen(cmd) == 0) continue;

        execute_pipeline(cmd);
    }

    return 0;
}
