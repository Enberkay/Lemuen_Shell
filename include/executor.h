#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

// Execute a command
int execute_command(command_t *cmd);

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

// Wait for background processes
void wait_for_background_processes(void);

// Signal handling for child processes
void setup_child_signal_handlers(void);

#endif // EXECUTOR_H
