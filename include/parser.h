#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

// Logical operator types
typedef enum {
    LOGIC_NONE = 0,
    LOGIC_AND,      // &&
    LOGIC_OR        // ||
} logic_operator_t;

// Command structure to hold parsed command
typedef struct {
    char **args;           // Array of arguments
    int argc;              // Number of arguments
    char *input_redirect;  // Input redirection file
    char *output_redirect; // Output redirection file
    int append_output;     // Whether to append (>>) or overwrite (>)
    char *next_command;    // For command chaining (;)
    int background;        // Whether to run in background (&)
    logic_operator_t logic_op;  // Logical operator (&&, ||)
    char *next_logic_command;   // Next command after logical operator
} command_t;

// Parse a command line string into command_t structure
command_t *parse_command(const char *line);

// Free command_t structure
void free_command(command_t *cmd);

// Check if command is empty or only whitespace
int is_empty_command(const char *line);

// Parse command chaining (;)
command_t **parse_command_chain(const char *line, int *count);

// Free command chain array
void free_command_chain(command_t **commands, int count);

// Parse logical operators (&&, ||)
command_t **parse_logical_chain(const char *line, int *count);

// Free logical chain array
void free_logical_chain(command_t **commands, int count);

#endif // PARSER_H
