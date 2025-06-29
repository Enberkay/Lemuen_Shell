// lemuen/src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

void handle_sigint(int sig) {
    (void)sig; // unused
    printf("\nlemuen> ");
    fflush(stdout);
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

        cmd[strcspn(cmd, "\n")] = '\0'; // remove newline

        if (strlen(cmd) == 0) continue;

        if (strcmp(cmd, "exit") == 0) break;

        char *argv[MAX_ARGS];
        int argc = 0;
        char *token = strtok(cmd, " ");

        while (token != NULL && argc < MAX_ARGS - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        pid_t pid = fork();

        if (pid == 0) {
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
