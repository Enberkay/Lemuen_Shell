// lemuen/src/main.c - Lemuen Shell v0.7
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h> // Required for errno

#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "utils.h"

#define PROMPT_COLOR "\001\033[1;36m\002"  // Cyan bold
#define RESET_COLOR  "\001\033[0m\002"

/**
 * handle_sigint - Signal handler for SIGINT (Ctrl+C).
 * @sig: Signal number.
 *
 * Resets the prompt line and redisplays it.
 */
void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

/**
 * handle_sigchld - Signal handler for SIGCHLD (Child process terminated).
 * @sig: Signal number.
 *
 * Cleans up background processes that have finished.
 */
void handle_sigchld(int sig) {
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
    errno = saved_errno;
}

/**
 * main - Entry point for Lemuen Shell.
 *
 * Initializes signal handling, readline, and runs the main shell loop.
 * Returns: Exit status code.
 */
int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);

    using_history();

    char *line;
    while ((line = readline(PROMPT_COLOR "lemuen> " RESET_COLOR)) != NULL) {
        if (*line) add_history(line);

        command_t *cmd = parse_command(line);
        if (cmd) {
            // Execute command (handles chaining, logical operators, etc.)
            execute_command(cmd);
            free_command(cmd);
        }
        free(line);
    }

    printf("\nBye from Lemuen Shell!\n");
    cleanup_find_command_cache();
    return 0;
}
