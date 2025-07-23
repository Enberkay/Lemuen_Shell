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

/**
 * execute_command - Entry point for executing a parsed command structure.
 * @cmd: Command to execute.
 *
 * Handles chaining, logical operators, and delegates to single command execution.
 * Returns: Exit status code.
 */
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

/**
 * execute_single_command - Execute a single command (builtin or external).
 * @cmd: Command to execute.
 *
 * Handles background execution, redirection, and builtin/external distinction.
 * Returns: Exit status code.
 */
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

    // Handle builtin commands without redirection/background directly
    if (is_builtin(cmd) && !cmd->input_redirect && !cmd->output_redirect && !cmd->background) {
        return run_builtin(cmd);
    }

    // Handle background execution
    if (cmd->background) {
        return execute_background(cmd);
    }

    // Handle redirections for all commands (both builtin and external)
    if (cmd->input_redirect || cmd->output_redirect) {
        return execute_with_redirection(cmd);
    }

    // No redirection - handle external
    return execute_external(cmd);
}

/**
 * execute_with_redirection - Execute a command with I/O redirection in a child process.
 * @cmd: Command to execute.
 *
 * Handles input/output file redirection and executes builtin or external command.
 * Returns: Exit status code.
 */
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

/**
 * execute_background - Execute a command in the background (asynchronously).
 * @cmd: Command to execute.
 *
 * Forks a new process group and does not wait for completion.
 * Returns: 0 on success, 1 on error.
 */
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

/**
 * find_command - Search for an executable in PATH or as a direct path.
 * @command: Command name or path.
 *
 * Returns: Newly allocated string with full path, or NULL if not found.
 *
 * Optimization: Cache split $PATH result and only re-split if $PATH changes.
 */
char *find_command(const char *command) {
    if (!command) return NULL;

    // If command contains '/', treat as absolute or relative path
    if (strchr(command, '/')) {
        if (is_executable(command)) {
            return strdup_safe(command);
        }
        return NULL;
    }

    // --- Optimization: cache split $PATH ---
    static char **cached_paths = NULL;
    static char *cached_path_env = NULL;
    static int cached_path_count = 0;
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return NULL;
    }
    if (!cached_path_env || strcmp(cached_path_env, path_env) != 0) {
        // $PATH changed, re-split
        if (cached_paths) {
            free_string_array(cached_paths);
            cached_paths = NULL;
        }
        if (cached_path_env) {
            free(cached_path_env);
            cached_path_env = NULL;
        }
        cached_path_env = strdup_safe(path_env);
        cached_paths = split_string(path_env, ":", &cached_path_count);
    }
    if (!cached_paths) {
        return NULL;
    }
    // --- End optimization ---

    char *found_path = NULL;
    for (int i = 0; i < cached_path_count; i++) {
        char *full_path = malloc(strlen(cached_paths[i]) + strlen(command) + 2);
        if (!full_path) continue;
        sprintf(full_path, "%s/%s", cached_paths[i], command);
        if (is_executable(full_path)) {
            found_path = full_path;
            break;
        }
        free(full_path);
    }
    return found_path;
}

/**
 * is_executable - Check if a file is executable.
 * @path: Path to check.
 *
 * Returns: 1 if executable, 0 otherwise.
 */
int is_executable(const char *path) {
    if (!path) return 0;
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

/**
 * execute_external - Execute an external (non-builtin) command.
 * @cmd: Command to execute.
 *
 * Forks and execs the command, waits for completion.
 * Returns: Exit status code.
 */
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

/**
 * execute_command_chain - Execute a chain of commands sequentially.
 * @commands: Array of command pointers.
 * @count: Number of commands.
 *
 * Returns: Exit status of the last command.
 */
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

/**
 * wait_for_background_processes - Reap all finished background processes.
 */
void wait_for_background_processes(void) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Background process finished
    }
}

/**
 * setup_child_signal_handlers - Reset signal handlers in child process to default.
 */
void setup_child_signal_handlers(void) {
    // Reset signal handlers for child processes
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

/**
 * execute_with_logical - Execute a command with logical operators (&&, ||).
 * @cmd: Command to execute.
 *
 * Returns: Exit status code.
 */
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

/**
 * execute_logical_chain - Execute a chain of commands with logical operators.
 * @commands: Array of command pointers.
 * @count: Number of commands.
 *
 * Returns: Exit status of the last command.
 */
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
