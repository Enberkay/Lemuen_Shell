#ifndef BUILTINS_H
#define BUILTINS_H

#include "parser.h"

// Builtin command function type
typedef int (*builtin_func_t)(command_t *cmd);

// Builtin command structure
typedef struct {
    const char *name;
    builtin_func_t func;
    const char *help;
} builtin_t;

// Check if command is a builtin
int is_builtin(command_t *cmd);

// Run builtin command
int run_builtin(command_t *cmd);

// Builtin command implementations
int builtin_cd(command_t *cmd);
int builtin_exit(command_t *cmd);
int builtin_pwd(command_t *cmd);
int builtin_echo(command_t *cmd);
int builtin_help(command_t *cmd);
int builtin_export(command_t *cmd);
int builtin_unset(command_t *cmd);

// Get list of all builtins
const builtin_t *get_builtins(void);

// Get number of builtins
int get_builtin_count(void);

#endif // BUILTINS_H
