#include "executor.h"
#include "builtins.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int execute_command(command_t *cmd) {
    if (!cmd || !cmd->args || cmd->argc == 0) {
        return 1;
    }

    // Expand environment variables in command arguments
    expand_env_vars(cmd);

    // Handle command chaining (;)
    if (cmd->next_command) {
        int status = execute_single_command(cmd);
        command_t *next_cmd = parse_command(cmd->next_command);
        if (next_cmd) {
            int next_status = execute_command(next_cmd);
            free_command(next_cmd);
            return next_status;
        }
        return status;
    }

    // Handle logical operators (&&, ||)
    if (cmd->logic_op != LOGIC_NONE && cmd->next_logic_command) {
        return execute_with_logical(cmd);
    }

    // Execute single command
    return execute_single_command(cmd);
}

int execute_single_command(command_t *cmd) {
    if (!cmd) {
        return 1;
    }
    
    // Handle empty command
    if (!cmd->args || cmd->argc == 0) {
        return 0;  // Empty command succeeds
    }

    // Expand environment variables in command arguments
    expand_env_vars(cmd);

    // Handle background execution
    if (cmd->background) {
        return execute_background(cmd);
    }

    // Handle redirections for all commands (both builtin and external)
    if (cmd->input_redirect || cmd->output_redirect) {
        return execute_with_redirection(cmd);
    }

    // No redirection - handle builtin or external
    if (is_builtin(cmd)) {
        return run_builtin(cmd);
    }
    return execute_external(cmd);
}

int execute_with_redirection(command_t *cmd) {
    pid_t pid = fork();
    
    if (pid == -1) {
        print_system_error("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->input_redirect) {
            int fd = open(cmd->input_redirect, O_RDONLY);
            if (fd == -1) {
                print_system_error("failed to open input file");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                print_system_error("failed to redirect input");
                close(fd);
                exit(1);
            }
            close(fd);
        }
        
        // Handle output redirection
        if (cmd->output_redirect) {
            int flags = O_WRONLY | O_CREAT;
            flags |= cmd->append_output ? O_APPEND : O_TRUNC;
            
            int fd = open(cmd->output_redirect, flags, 0644);
            if (fd == -1) {
                print_system_error("failed to open output file");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                print_system_error("failed to redirect output");
                close(fd);
                exit(1);
            }
            close(fd);
        }
        
        // Execute the command (builtin or external)
        if (is_builtin(cmd)) {
            int ret = run_builtin(cmd);
            fflush(stdout);
            exit(ret);
        } else {
            execvp(cmd->args[0], cmd->args);
            print_system_error("exec failed");
            exit(1);
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

int execute_background(command_t *cmd) {
    pid_t pid = fork();
    
    if (pid == -1) {
        print_system_error("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Child process - run in background
        setpgid(0, 0);  // Create new process group
        
        if (cmd->input_redirect || cmd->output_redirect) {
            execute_with_redirection(cmd);
        } else {
            execvp(cmd->args[0], cmd->args);
            print_system_error("exec failed");
        }
        exit(1);
    } else {
        // Parent process
        printf("[%d] %s\n", pid, cmd->args[0]);
        return 0;
    }
}

char *find_command(const char *command) {
    if (!command) return NULL;
    
    // If command contains '/', treat as absolute or relative path
    if (strchr(command, '/')) {
        if (is_executable(command)) {
            return strdup_safe(command);
        }
        return NULL;
    }
    
    // Search in PATH
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return NULL;
    }
    
    int path_count;
    char **paths = split_string(path_env, ":", &path_count);
    if (!paths) {
        return NULL;
    }
    
    char *found_path = NULL;
    for (int i = 0; i < path_count; i++) {
        char *full_path = malloc(strlen(paths[i]) + strlen(command) + 2);
        if (!full_path) continue;
        
        sprintf(full_path, "%s/%s", paths[i], command);
        if (is_executable(full_path)) {
            found_path = full_path;
            break;
        }
        free(full_path);
    }
    
    free_string_array(paths);
    return found_path;
}

int is_executable(const char *path) {
    if (!path) return 0;
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

int execute_external(command_t *cmd) {
    char *command_path = find_command(cmd->args[0]);
    if (!command_path) {
        print_error("command not found: %s", cmd->args[0]);
        return 127;  // Command not found
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        print_system_error("fork failed");
        free(command_path);
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        execv(command_path, cmd->args);
        print_system_error("exec failed");
        free(command_path);
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        free(command_path);
        return WEXITSTATUS(status);
    }
}

int execute_command_chain(command_t **commands, int count) {
    if (!commands || count <= 0) {
        return 0;
    }
    
    int last_status = 0;
    for (int i = 0; i < count; i++) {
        if (commands[i]) {
            last_status = execute_command(commands[i]);
        }
    }
    
    return last_status;
}

void wait_for_background_processes(void) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Background process finished
    }
}

void setup_child_signal_handlers(void) {
    // Reset signal handlers for child processes
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

int execute_with_logical(command_t *cmd) {
    if (!cmd) return 1;
    
    // Execute the current command (without logical operators to avoid recursion)
    int status = execute_single_command(cmd);
    

    
    // If there's a logical operator, handle it
    if (cmd->logic_op != LOGIC_NONE && cmd->next_logic_command) {
        command_t *next_cmd = parse_command(cmd->next_logic_command);
        if (next_cmd) {
            if (cmd->logic_op == LOGIC_AND) {
                // && : execute next command only if current command succeeded
                if (status == 0) {
                    status = execute_command(next_cmd);
                }
            } else if (cmd->logic_op == LOGIC_OR) {
                // || : execute next command only if current command failed
                if (status != 0) {
                    status = execute_command(next_cmd);
                }
            }
            free_command(next_cmd);
        }
    }
    
    // Handle command chaining after logical operators
    if (cmd->next_command) {
        command_t *chain_cmd = parse_command(cmd->next_command);
        if (chain_cmd) {
            int chain_status = execute_command(chain_cmd);
            free_command(chain_cmd);
            return chain_status;
        }
    }
    
    return status;
}

int execute_logical_chain(command_t **commands, int count) {
    if (!commands || count <= 0) {
        return 0;
    }
    
    int last_status = 0;
    for (int i = 0; i < count; i++) {
        if (commands[i]) {
            last_status = execute_with_logical(commands[i]);
        }
    }
    
    return last_status;
}
