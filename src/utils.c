#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

char *strdup_safe(const char *str) {
    if (!str) return NULL;
    char *dup = strdup(str);
    if (!dup) {
        print_error("strdup failed: out of memory");
        exit(1);
    }
    return dup;
}

char *trim_left(char *str) {
    if (!str) return NULL;
    
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) {
        str++;
    }
    return str;
}

char *trim_right(char *str) {
    if (!str) return NULL;
    
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

char *trim(char *str) {
    if (!str) return NULL;
    str = trim_left(str);
    return trim_right(str);
}

char **split_string(const char *str, const char *delim, int *count) {
    if (!str || !delim || !count) return NULL;
    
    char *str_copy = strdup_safe(str);
    char *token;
    char **tokens = NULL;
    int capacity = 10;
    int size = 0;
    
    tokens = malloc(capacity * sizeof(char *));
    if (!tokens) {
        free(str_copy);
        return NULL;
    }
    
    token = strtok(str_copy, delim);
    while (token) {
        if (size >= capacity) {
            capacity *= 2;
            char **new_tokens = realloc(tokens, capacity * sizeof(char *));
            if (!new_tokens) {
                free_string_array(tokens);
                free(str_copy);
                return NULL;
            }
            tokens = new_tokens;
        }
        
        tokens[size++] = strdup_safe(token);
        token = strtok(NULL, delim);
    }
    
    // Null-terminate the array
    if (size >= capacity) {
        capacity += 1;
        char **new_tokens = realloc(tokens, capacity * sizeof(char *));
        if (!new_tokens) {
            free_string_array(tokens);
            free(str_copy);
            return NULL;
        }
        tokens = new_tokens;
    }
    tokens[size] = NULL;
    
    *count = size;
    free(str_copy);
    return tokens;
}

void free_string_array(char **array) {
    if (!array) return;
    
    for (int i = 0; array[i] != NULL; i++) {
        free(array[i]);
    }
    free(array);
}

char *get_env_var(const char *name) {
    if (!name) return NULL;
    return getenv(name);
}

void set_env_var(const char *name, const char *value) {
    if (!name) return;
    
    if (value) {
        setenv(name, value, 1);
    } else {
        unsetenv(name);
    }
}

char *expand_tilde(const char *path) {
    if (!path || path[0] != '~') {
        return strdup_safe(path);
    }
    
    if (path[1] == '\0' || path[1] == '/') {
        // Expand ~ to home directory
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/";
        }
        
        if (path[1] == '\0') {
            return strdup_safe(home);
        } else {
            char *expanded = malloc(strlen(home) + strlen(path));
            if (!expanded) return NULL;
            sprintf(expanded, "%s%s", home, path + 1);
            return expanded;
        }
    }
    
    return strdup_safe(path);
}

char *get_current_dir(void) {
    char *cwd = getcwd(NULL, 0);
    if (!cwd) {
        print_system_error("getcwd failed");
        return strdup_safe(".");
    }
    return cwd;
}

void print_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[1;31mlemuen: \033[0m");  // Red "lemuen: "
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void print_system_error(const char *message) {
    print_error("%s: %s", message, strerror(errno));
}

// Expand environment variables in a string
char *expand_env_var_in_string(const char *str) {
    if (!str) return NULL;
    
    char *result = malloc(1);
    if (!result) return NULL;
    result[0] = '\0';
    size_t result_len = 0;
    
    const char *p = str;
    while (*p) {
        if (*p == '$' && (p == str || *(p-1) != '\\')) {
            // Found $, check if it's an environment variable
            p++; // Skip $
            
            if (*p == '{') {
                // ${VAR} format
                p++; // Skip {
                const char *var_start = p;
                while (*p && *p != '}') p++;
                
                if (*p == '}') {
                    int var_len = p - var_start;
                    char *var_name = malloc(var_len + 1);
                    if (var_name) {
                        strncpy(var_name, var_start, var_len);
                        var_name[var_len] = '\0';
                        
                        const char *var_value = getenv(var_name);
                        if (var_value) {
                            size_t value_len = strlen(var_value);
                            char *new_result = realloc(result, result_len + value_len + 1);
                            if (new_result) {
                                result = new_result;
                                strcpy(result + result_len, var_value);
                                result_len += value_len;
                            }
                        }
                        free(var_name);
                    }
                    p++; // Skip }
                } else {
                    // Invalid ${ format, keep original
                    char *new_result = realloc(result, result_len + 2);
                    if (new_result) {
                        result = new_result;
                        result[result_len++] = '$';
                        result[result_len++] = '{';
                        result[result_len] = '\0';
                    }
                    p = var_start - 1; // Go back to before {
                }
            } else if (isalnum(*p) || *p == '_') {
                // $VAR format
                const char *var_start = p;
                while (*p && (isalnum(*p) || *p == '_')) p++;
                
                int var_len = p - var_start;
                char *var_name = malloc(var_len + 1);
                if (var_name) {
                    strncpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    const char *var_value = getenv(var_name);
                    if (var_value) {
                        size_t value_len = strlen(var_value);
                        char *new_result = realloc(result, result_len + value_len + 1);
                        if (new_result) {
                            result = new_result;
                            strcpy(result + result_len, var_value);
                            result_len += value_len;
                        }
                    }
                    free(var_name);
                }
                p--; // Go back one character since we'll increment in the loop
            } else {
                // Just a $, keep it
                char *new_result = realloc(result, result_len + 2);
                if (new_result) {
                    result = new_result;
                    result[result_len++] = '$';
                    result[result_len] = '\0';
                }
                p--; // Go back one character since we'll increment in the loop
            }
        } else {
            // Regular character
            char *new_result = realloc(result, result_len + 2);
            if (new_result) {
                result = new_result;
                result[result_len++] = *p;
                result[result_len] = '\0';
            }
        }
        p++;
    }
    
    return result;
}

// Expand environment variables in command arguments
void expand_env_vars(command_t *cmd) {
    if (!cmd || !cmd->args) return;
    
    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->args[i]) {
            char *expanded = expand_env_var_in_string(cmd->args[i]);
            if (expanded) {
                free(cmd->args[i]);
                cmd->args[i] = expanded;
            }
        }
    }
}
