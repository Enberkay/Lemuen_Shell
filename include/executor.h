#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

// Execute a command
int execute_command(command_t *cmd);

// Execute a single command (without chaining/logical operators)
int execute_single_command(command_t *cmd);

// Execute command with redirection
int execute_with_redirection(command_t *cmd);

// Execute command in background
int execute_background(command_t *cmd);

// Find command in PATH
char *find_command(const char *command);

// Check if file is executable
int is_executable(const char *path);

// Execute external command
int execute_external(command_t *cmd);

// Handle command chaining
int execute_command_chain(command_t **commands, int count);

// Handle logical operators (&&, ||)
int execute_logical_chain(command_t **commands, int count);

// Execute command with logical operator
int execute_with_logical(command_t *cmd);

// Wait for background processes
void wait_for_background_processes(void);

// Signal handling for child processes
void setup_child_signal_handlers(void);

void cleanup_find_command_cache(void);

#endif // EXECUTOR_H
