// lemuen/src/main.c - Lemuen Shell v0.5

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "utils.h"

#define PROMPT_COLOR "\001\033[1;36m\002"  // Cyan bold
#define RESET_COLOR  "\001\033[0m\002"

void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

int main() {
    signal(SIGINT, handle_sigint);

    using_history();

    char *line;
    while ((line = readline(PROMPT_COLOR "lemuen> " RESET_COLOR)) != NULL) {
        if (*line) add_history(line);

        command_t *cmd = parse_command(line);
        if (cmd) {
            if (cmd->input_redirect || cmd->output_redirect) {
                // Always use execute_command for redirection
                execute_command(cmd);
            } else if (is_builtin(cmd)) {
                run_builtin(cmd);
            } else {
                execute_command(cmd);
            }
            free_command(cmd);
        }
        free(line);
    }

    printf("\nBye from Lemuen Shell!\n");
    return 0;
}
