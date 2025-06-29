#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

command_t *parse_command(const char *line) {
    if (!line || is_empty_command(line)) {
        return NULL;
    }
    
    command_t *cmd = calloc(1, sizeof(command_t));
    if (!cmd) {
        print_error("Failed to allocate command structure");
        return NULL;
    }
    
    char *line_copy = strdup_safe(line);
    char *trimmed = trim(line_copy);
    
    // Check for background execution
    int len = strlen(trimmed);
    if (len > 0 && trimmed[len-1] == '&') {
        cmd->background = 1;
        trimmed[len-1] = '\0';
        trim(trimmed);
    }
    
    // Check for command chaining
    char *semicolon = strchr(trimmed, ';');
    if (semicolon) {
        *semicolon = '\0';
        cmd->next_command = strdup_safe(semicolon + 1);
        trim(cmd->next_command);
    }
    
    // Parse redirections
    char *input_redir = strstr(trimmed, "<");
    char *output_redir = strstr(trimmed, ">");
    char *append_redir = strstr(trimmed, ">>");
    
    if (input_redir) {
        *input_redir = '\0';
        cmd->input_redirect = strdup_safe(trim(input_redir + 1));
    }
    
    if (append_redir) {
        *append_redir = '\0';
        cmd->output_redirect = strdup_safe(trim(append_redir + 2));
        cmd->append_output = 1;
    } else if (output_redir) {
        *output_redir = '\0';
        cmd->output_redirect = strdup_safe(trim(output_redir + 1));
        cmd->append_output = 0;
    }
    
    // Parse arguments
    trim(trimmed);
    if (strlen(trimmed) > 0) {
        int arg_count;
        char **args = split_string(trimmed, " \t", &arg_count);
        if (args) {
            cmd->args = args;
            cmd->argc = arg_count;
        }
    }
    
    free(line_copy);
    return cmd;
}

void free_command(command_t *cmd) {
    if (!cmd) return;
    
    if (cmd->args) {
        free_string_array(cmd->args);
    }
    
    free(cmd->input_redirect);
    free(cmd->output_redirect);
    free(cmd->next_command);
    free(cmd);
}

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

void free_command_chain(command_t **commands, int count) {
    if (!commands) return;
    
    for (int i = 0; i < count; i++) {
        free_command(commands[i]);
    }
    free(commands);
}
