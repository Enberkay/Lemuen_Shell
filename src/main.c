// lemuen/src/main.c - Lemuen Shell v0.2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

void handle_sigint(int sig) {
    (void)sig; // unused
    printf("\nlemuen> ");
    fflush(stdout);
}

// ฟังก์ชันแยกคำสั่ง + redirection
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
        if (strcmp(cmd, "exit") == 0) break;

        char *argv[MAX_ARGS];
        char *infile = NULL, *outfile = NULL;
        int append = 0;

        parse_command(cmd, argv, &infile, &outfile, &append);

        pid_t pid = fork();

        if (pid == 0) {
            if (infile) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) {
                    perror("lemuen input");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (outfile) {
                int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                int fd = open(outfile, flags, 0644);
                if (fd < 0) {
                    perror("lemuen output");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(argv[0], argv);
            perror("lemuen");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }

    return 0;
}
