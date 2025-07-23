#define _GNU_SOURCE
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * parse_command - Parse a command line string into a command_t structure.
 * @line: Input command line.
 *
 * Handles logical operators, pipelines, redirection, background, and argument splitting.
 * Returns: Pointer to parsed command_t, or NULL on error.
 */
// Helper: parse redirection in-place, set fields and null-terminate
static void parse_redirection(char *cmd_str, command_t *cmd) {
    // Output append (>>)
    char *append_redir = strstr(cmd_str, ">>");
    if (append_redir) {
        *append_redir = '\0';
        append_redir += 2;
        while (*append_redir == ' ' || *append_redir == '\t') append_redir++;
        cmd->output_redirect = strdup_safe(append_redir);
        cmd->append_output = 1;
    } else {
        // Output overwrite (>), but not >>
        char *output_redir = strchr(cmd_str, '>');
        if (output_redir) {
            *output_redir = '\0';
            output_redir++;
            while (*output_redir == ' ' || *output_redir == '\t') output_redir++;
            cmd->output_redirect = strdup_safe(output_redir);
            cmd->append_output = 0;
        }
    }
    // Input (<)
    char *input_redir = strchr(cmd_str, '<');
    if (input_redir) {
        *input_redir = '\0';
        input_redir++;
        while (*input_redir == ' ' || *input_redir == '\t') input_redir++;
        cmd->input_redirect = strdup_safe(input_redir);
    }
}

// Helper: parse background (&) in-place
static void parse_background(char *cmd_str, command_t *cmd) {
    size_t len = strlen(cmd_str);
    if (len > 0 && cmd_str[len-1] == '&') {
        cmd->background = 1;
        cmd_str[len-1] = '\0';
        // Remove trailing whitespace
        trim(cmd_str);
    }
}

// Helper: parse arguments in-place
static void parse_args(char *cmd_str, command_t *cmd) {
    trim(cmd_str);
    if (strlen(cmd_str) > 0) {
        int arg_count;
        char **args = split_string(cmd_str, " \t", &arg_count);
        if (args) {
            cmd->args = args;
            cmd->argc = arg_count;
        }
    } else {
        cmd->args = NULL;
        cmd->argc = 0;
    }
}

command_t *parse_command(const char *line) {
    if (!line || is_empty_command(line)) {
        return NULL;
    }
    // Work on a single buffer for the whole function
    char *buffer = strdup_safe(line);
    trim(buffer);
    // Check for logical operators (&&, ||)
    char *and_op = strstr(buffer, "&&");
    char *or_op = strstr(buffer, "||");
    if (and_op || or_op) {
        command_t *cmd = calloc(1, sizeof(command_t));
        if (!cmd) {
            print_error("Failed to allocate command structure");
            free(buffer);
            return NULL;
        }
        if (and_op && (!or_op || and_op < or_op)) {
            *and_op = '\0';
            cmd->logic_op = LOGIC_AND;
            char *next_part = and_op + 2;
            trim(next_part);
            // Check for command chaining
            char *semicolon = strchr(next_part, ';');
            if (semicolon) {
                *semicolon = '\0';
                cmd->next_logic_command = strdup_safe(trim(next_part));
                cmd->next_command = strdup_safe(trim(semicolon + 1));
            } else {
                cmd->next_logic_command = strdup_safe(trim(next_part));
            }
        } else if (or_op) {
            *or_op = '\0';
            cmd->logic_op = LOGIC_OR;
            char *next_part = or_op + 2;
            trim(next_part);
            char *semicolon = strchr(next_part, ';');
            if (semicolon) {
                *semicolon = '\0';
                cmd->next_logic_command = strdup_safe(trim(next_part));
                cmd->next_command = strdup_safe(trim(semicolon + 1));
            } else {
                cmd->next_logic_command = strdup_safe(trim(next_part));
            }
        }
        // Parse first part (before && or ||)
        char *first_part = trim(buffer);
        parse_background(first_part, cmd);
        parse_redirection(first_part, cmd);
        parse_args(first_part, cmd);
        free(buffer);
        return cmd;
    }
    // Pipeline split (|) - only if no logical operators
    char *pipe_saveptr = NULL;
    char *pipe_token = strtok_r(buffer, "|", &pipe_saveptr);
    command_t *first_cmd = NULL;
    command_t *last_cmd = NULL;
    while (pipe_token) {
        command_t *cmd = calloc(1, sizeof(command_t));
        if (!cmd) {
            print_error("Failed to allocate command structure");
            if (first_cmd) free_command(first_cmd);
            free(buffer);
            return NULL;
        }
        char *segment = trim(pipe_token);
        parse_background(segment, cmd);
        parse_redirection(segment, cmd);
        parse_args(segment, cmd);
        if (!first_cmd) {
            first_cmd = cmd;
        } else {
            last_cmd->next_pipe = cmd;
        }
        last_cmd = cmd;
        pipe_token = strtok_r(NULL, "|", &pipe_saveptr);
    }
    free(buffer);
    return first_cmd;
}

/**
 * free_command - Free all memory associated with a command_t structure.
 * @cmd: Command to free.
 */
void free_command(command_t *cmd) {
    if (!cmd) return;
    
    if (cmd->args) {
        free_string_array(cmd->args);
    }
    
    free(cmd->input_redirect);
    free(cmd->output_redirect);
    free(cmd->next_command);
    free(cmd->next_logic_command);
    free(cmd);
}

/**
 * is_empty_command - Check if a command line is empty or whitespace only.
 * @line: Input string.
 *
 * Returns: 1 if empty, 0 otherwise.
 */
int is_empty_command(const char *line) {
    if (!line) return 1;
    
    while (*line) {
        if (*line != ' ' && *line != '\t' && *line != '\n') {
            return 0;
        }
        line++;
    }
    return 1;
}

/**
 * parse_command_chain - Parse a chain of commands separated by ';'.
 * @line: Input command line.
 * @count: Output pointer for number of commands.
 *
 * Returns: Array of command_t pointers.
 */
command_t **parse_command_chain(const char *line, int *count) {
    if (!line || !count) return NULL;
    
    char *line_copy = strdup_safe(line);
    int chain_count;
    char **commands = split_string(line_copy, ";", &chain_count);
    
    if (!commands) {
        free(line_copy);
        *count = 0;
        return NULL;
    }
    
    command_t **parsed_commands = malloc(chain_count * sizeof(command_t *));
    if (!parsed_commands) {
        free_string_array(commands);
        free(line_copy);
        *count = 0;
        return NULL;
    }
    
    int valid_count = 0;
    for (int i = 0; i < chain_count; i++) {
        trim(commands[i]);
        if (strlen(commands[i]) > 0) {
            parsed_commands[valid_count] = parse_command(commands[i]);
            if (parsed_commands[valid_count]) {
                valid_count++;
            }
        }
    }
    
    free_string_array(commands);
    free(line_copy);
    *count = valid_count;
    return parsed_commands;
}

/**
 * free_command_chain - Free an array of command_t pointers.
 * @commands: Array to free.
 * @count: Number of commands.
 */
void free_command_chain(command_t **commands, int count) {
    if (!commands) return;
    
    for (int i = 0; i < count; i++) {
        free_command(commands[i]);
    }
    free(commands);
}

/**
 * parse_logical_chain - Parse a chain of commands separated by '&&' or '||'.
 * @line: Input command line.
 * @count: Output pointer for number of commands.
 *
 * Returns: Array of command_t pointers.
 */
command_t **parse_logical_chain(const char *line, int *count) {
    if (!line || !count) return NULL;
    
    char *line_copy = strdup_safe(line);
    int chain_count;
    char **commands = split_string(line_copy, "&&", &chain_count);
    
    if (!commands) {
        // Try splitting by ||
        commands = split_string(line_copy, "||", &chain_count);
        if (!commands) {
            free(line_copy);
            *count = 0;
            return NULL;
        }
    }
    
    command_t **parsed_commands = malloc(chain_count * sizeof(command_t *));
    if (!parsed_commands) {
        free_string_array(commands);
        free(line_copy);
        *count = 0;
        return NULL;
    }
    
    int valid_count = 0;
    for (int i = 0; i < chain_count; i++) {
        trim(commands[i]);
        if (strlen(commands[i]) > 0) {
            parsed_commands[valid_count] = parse_command(commands[i]);
            if (parsed_commands[valid_count]) {
                valid_count++;
            }
        }
    }
    
    free_string_array(commands);
    free(line_copy);
    *count = valid_count;
    return parsed_commands;
}

/**
 * free_logical_chain - Free an array of command_t pointers for logical chains.
 * @commands: Array to free.
 * @count: Number of commands.
 */
void free_logical_chain(command_t **commands, int count) {
    if (!commands) return;
    
    for (int i = 0; i < count; i++) {
        free_command(commands[i]);
    }
    free(commands);
}
