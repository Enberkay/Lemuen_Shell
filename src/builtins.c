#include "builtins.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Global variable to store previous directory
static char *previous_dir = NULL;

// Builtin command implementations
static int builtin_cd_impl(command_t *cmd);
static int builtin_exit_impl(command_t *cmd);
static int builtin_pwd_impl(command_t *cmd);
static int builtin_echo_impl(command_t *cmd);
static int builtin_help_impl(command_t *cmd);
static int builtin_export_impl(command_t *cmd);
static int builtin_unset_impl(command_t *cmd);

// Builtin commands table
static const builtin_t builtins[] = {
    {"cd", builtin_cd_impl, "cd [directory] - Change directory"},
    {"exit", builtin_exit_impl, "exit [n] - Exit shell with status n"},
    {"pwd", builtin_pwd_impl, "pwd - Print working directory"},
    {"echo", builtin_echo_impl, "echo [args...] - Print arguments"},
    {"help", builtin_help_impl, "help [command] - Show help"},
    {"export", builtin_export_impl, "export name=value - Set environment variable"},
    {"unset", builtin_unset_impl, "unset name - Unset environment variable"},
    {NULL, NULL, NULL}  // Sentinel
};

/**
 * is_builtin - Check if a command is a builtin command.
 * @cmd: Command to check.
 *
 * Returns: 1 if builtin, 0 otherwise.
 */
int is_builtin(command_t *cmd) {
    if (!cmd || !cmd->args || cmd->argc == 0) {
        return 0;
    }
    
    const char *command_name = cmd->args[0];
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(command_name, builtins[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * run_builtin - Execute a builtin command.
 * @cmd: Command to execute.
 *
 * Returns: Exit status code.
 */
int run_builtin(command_t *cmd) {
    if (!cmd || !cmd->args || cmd->argc == 0) {
        return 1;
    }
    
    const char *command_name = cmd->args[0];
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(command_name, builtins[i].name) == 0) {
            return builtins[i].func(cmd);
        }
    }
    
    return 1;  // Not found
}

/**
 * builtin_cd_impl - Implementation of the 'cd' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_cd_impl(command_t *cmd) {
    const char *target_dir = NULL;
    
    if (cmd->argc == 1) {
        // cd without arguments - go to home directory
        target_dir = getenv("HOME");
        if (!target_dir) {
            print_error("cd: HOME not set");
            return 1;
        }
    } else if (cmd->argc == 2) {
        if (strcmp(cmd->args[1], "-") == 0) {
            // cd - : go to previous directory
            if (!previous_dir) {
                print_error("cd: no previous directory");
                return 1;
            }
            target_dir = previous_dir;
        } else {
            target_dir = cmd->args[1];
        }
    } else {
        print_error("cd: too many arguments");
        return 1;
    }
    
    // Get current directory before changing
    char *current_dir = get_current_dir();
    
    // Expand tilde if present
    char *expanded_dir = expand_tilde(target_dir);
    if (!expanded_dir) {
        print_error("cd: failed to expand path");
        free(current_dir);
        return 1;
    }
    
    if (chdir(expanded_dir) != 0) {
        print_system_error("cd: failed to change directory");
        free(expanded_dir);
        free(current_dir);
        return 1;
    }
    
    // Update previous directory
    if (previous_dir) {
        free(previous_dir);
    }
    previous_dir = current_dir;
    
    free(expanded_dir);
    return 0;
}

/**
 * builtin_exit_impl - Implementation of the 'exit' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code (does not return).
 */
static int builtin_exit_impl(command_t *cmd) {
    int exit_code = 0;
    
    if (cmd->argc > 2) {
        print_error("exit: too many arguments");
        return 1;
    } else if (cmd->argc == 2) {
        exit_code = atoi(cmd->args[1]);
    }
    
    printf("Bye from Lemuen Shell!\n");
    exit(exit_code);
}

/**
 * builtin_pwd_impl - Implementation of the 'pwd' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_pwd_impl(command_t *cmd) {
    (void)cmd;  // Unused parameter
    
    char *cwd = get_current_dir();
    if (cwd) {
        printf("%s\n", cwd);
        free(cwd);
        return 0;
    }
    return 1;
}

/**
 * builtin_echo_impl - Implementation of the 'echo' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_echo_impl(command_t *cmd) {
    for (int i = 1; i < cmd->argc; i++) {
        if (i > 1) write(STDOUT_FILENO, " ", 1);
        if (cmd->args[i]) {
            write(STDOUT_FILENO, cmd->args[i], strlen(cmd->args[i]));
        }
    }
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}

/**
 * builtin_help_impl - Implementation of the 'help' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_help_impl(command_t *cmd) {
    if (cmd->argc == 1) {
        printf("Lemuen Shell v0.5 - Available builtin commands:\n");
        printf("==============================================\n");
        for (int i = 0; builtins[i].name; i++) {
            printf("  %s\n", builtins[i].help);
        }
        printf("\nFor more information about a command, type: help <command>\n");
    } else if (cmd->argc == 2) {
        const char *command_name = cmd->args[1];
        for (int i = 0; builtins[i].name; i++) {
            if (strcmp(command_name, builtins[i].name) == 0) {
                printf("%s\n", builtins[i].help);
                return 0;
            }
        }
        print_error("help: no help topics match '%s'", command_name);
        return 1;
    } else {
        print_error("help: too many arguments");
        return 1;
    }
    return 0;
}

/**
 * builtin_export_impl - Implementation of the 'export' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_export_impl(command_t *cmd) {
    if (cmd->argc != 2) {
        print_error("export: usage: export name=value");
        return 1;
    }
    
    const char *assignment = cmd->args[1];
    char *equals = strchr(assignment, '=');
    
    if (!equals) {
        print_error("export: invalid format, use: name=value");
        return 1;
    }
    
    int name_len = equals - assignment;
    char *name = malloc(name_len + 1);
    if (!name) {
        print_error("export: out of memory");
        return 1;
    }
    
    strncpy(name, assignment, name_len);
    name[name_len] = '\0';
    
    const char *value = equals + 1;
    set_env_var(name, value);
    
    free(name);
    return 0;
}

/**
 * builtin_unset_impl - Implementation of the 'unset' builtin command.
 * @cmd: Command structure.
 *
 * Returns: Exit status code.
 */
static int builtin_unset_impl(command_t *cmd) {
    if (cmd->argc != 2) {
        print_error("unset: usage: unset name");
        return 1;
    }
    
    set_env_var(cmd->args[1], NULL);
    return 0;
}

/**
 * get_builtins - Get the builtin commands table.
 *
 * Returns: Pointer to builtin_t array.
 */
const builtin_t *get_builtins(void) {
    return builtins;
}

/**
 * get_builtin_count - Get the number of builtin commands.
 *
 * Returns: Number of builtins.
 */
int get_builtin_count(void) {
    int count = 0;
    while (builtins[count].name) {
        count++;
    }
    return count;
} 