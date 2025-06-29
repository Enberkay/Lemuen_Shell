#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

// Command structure to hold parsed command
typedef struct {
    char **args;           // Array of arguments
    int argc;              // Number of arguments
    char *input_redirect;  // Input redirection file
    char *output_redirect; // Output redirection file
    int append_output;     // Whether to append (>>) or overwrite (>)
    char *next_command;    // For command chaining (;)
    int background;        // Whether to run in background (&)
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

#endif // PARSER_H
