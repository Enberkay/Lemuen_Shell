#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

/**
 * strdup_safe - Duplicate a string with error checking.
 * @str: Input string to duplicate.
 *
 * Returns: Newly allocated duplicate string, or exits on failure.
 */
char *strdup_safe(const char *str) {
    if (!str) return NULL;
    char *dup = strdup(str);
    if (!dup) {
        print_error("strdup failed: out of memory");
        exit(1);
    }
    return dup;
}

/**
 * trim_left - Remove leading whitespace from a string (in place).
 * @str: String to trim.
 *
 * Returns: Pointer to the first non-whitespace character.
 */
char *trim_left(char *str) {
    if (!str) return NULL;
    
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) {
        str++;
    }
    return str;
}

/**
 * trim_right - Remove trailing whitespace from a string (in place).
 * @str: String to trim.
 *
 * Returns: Pointer to the trimmed string.
 */
char *trim_right(char *str) {
    if (!str) return NULL;
    
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

/**
 * trim - Remove leading and trailing whitespace from a string (in place).
 * @str: String to trim.
 *
 * Returns: Pointer to the trimmed string.
 */
char *trim(char *str) {
    if (!str) return NULL;
    str = trim_left(str);
    return trim_right(str);
}

/**
 * split_string - Split a string into tokens by delimiter (in-place, efficient).
 * @str: Input string (will be copied and modified).
 * @delim: Delimiter characters.
 * @count: Output pointer for number of tokens.
 *
 * Returns: NULL-terminated array of pointers to tokens (in a single buffer).
 *          Caller must free the returned array and the buffer (tokens[0]).
 */
char **split_string(const char *str, const char *delim, int *count) {
    if (!str || !delim || !count) return NULL;
    char *buffer = strdup_safe(str);
    int capacity = 16;
    int size = 0;
    char **tokens = malloc(capacity * sizeof(char *));
    if (!tokens) {
        free(buffer);
        return NULL;
    }
    char *saveptr = NULL;
    char *token = strtok_r(buffer, delim, &saveptr);
    while (token) {
        if (size >= capacity) {
            capacity *= 2;
            char **new_tokens = realloc(tokens, capacity * sizeof(char *));
            if (!new_tokens) {
                free(tokens);
                free(buffer);
                return NULL;
            }
            tokens = new_tokens;
        }
        tokens[size++] = token;
        token = strtok_r(NULL, delim, &saveptr);
    }
    if (size >= capacity) {
        capacity++;
        char **new_tokens = realloc(tokens, capacity * sizeof(char *));
        if (!new_tokens) {
            free(tokens);
            free(buffer);
            return NULL;
        }
        tokens = new_tokens;
    }
    tokens[size] = NULL;
    *count = size;
    return tokens;
}

/**
 * free_string_array - Free a NULL-terminated array of strings.
 * @array: Array to free.
 */
void free_string_array(char **array) {
    if (!array) return;
    
    for (int i = 0; array[i] != NULL; i++) {
        free(array[i]);
    }
    free(array);
}

/**
 * free_split_string - Free the array and buffer returned by split_string.
 * @tokens: Array to free.
 */
void free_split_string(char **tokens) {
    if (!tokens) return;
    free(tokens[0]); // buffer
    free(tokens);
}

/**
 * get_env_var - Get the value of an environment variable.
 * @name: Variable name.
 *
 * Returns: Value string or NULL if not found.
 */
char *get_env_var(const char *name) {
    if (!name) return NULL;
    return getenv(name);
}

/**
 * set_env_var - Set or unset an environment variable.
 * @name: Variable name.
 * @value: Value to set, or NULL to unset.
 */
void set_env_var(const char *name, const char *value) {
    if (!name) return;
    
    if (value) {
        setenv(name, value, 1);
    } else {
        unsetenv(name);
    }
}

/**
 * expand_tilde - Expand ~ to home directory in a path string.
 * @path: Input path.
 *
 * Returns: Newly allocated expanded path string.
 */
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

/**
 * get_current_dir - Get the current working directory.
 *
 * Returns: Newly allocated string with current directory path.
 */
char *get_current_dir(void) {
    char *cwd = getcwd(NULL, 0);
    if (!cwd) {
        print_system_error("getcwd failed");
        return strdup_safe(".");
    }
    return cwd;
}

/**
 * print_error - Print a formatted error message to stderr (in red).
 * @format: printf-style format string.
 * @...: Arguments.
 */
void print_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[1;31mlemuen: \033[0m");  // Red "lemuen: "
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * print_system_error - Print a system error message with strerror.
 * @message: Custom message to print before system error.
 */
void print_system_error(const char *message) {
    print_error("%s: %s", message, strerror(errno));
}

/**
 * expand_env_var_in_string - Expand environment variables in a string.
 * @str: Input string (may contain $VAR or ${VAR}).
 *
 * Returns: Newly allocated string with variables expanded.
 */
char *expand_env_var_in_string(const char *str) {
    if (!str) return NULL;
    
    size_t bufsize = 64;
    char *result = malloc(bufsize);
    if (!result) return NULL;
    result[0] = '\0';
    size_t result_len = 0;
    
    const char *p = str;
    while (*p) {
        if (*p == '$' && (p == str || *(p-1) != '\\')) {
            p++;
            if (*p == '{') {
                p++;
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
                            if (result_len + value_len + 1 > bufsize) {
                                while (result_len + value_len + 1 > bufsize) bufsize *= 2;
                                char *new_result = realloc(result, bufsize);
                                if (!new_result) { free(result); free(var_name); return NULL; }
                                result = new_result;
                            }
                            strcpy(result + result_len, var_value);
                            result_len += value_len;
                        }
                        free(var_name);
                    }
                    p++; // Skip }
                    continue;
                }
                // Invalid ${ format, treat as literal
                if (result_len + 2 > bufsize) {
                    bufsize *= 2;
                    char *new_result = realloc(result, bufsize);
                    if (!new_result) { free(result); return NULL; }
                    result = new_result;
                }
                result[result_len++] = '$';
                result[result_len++] = '{';
                result[result_len] = '\0';
                p = var_start - 1;
            } else if (isalnum(*p) || *p == '_') {
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
                        if (result_len + value_len + 1 > bufsize) {
                            while (result_len + value_len + 1 > bufsize) bufsize *= 2;
                            char *new_result = realloc(result, bufsize);
                            if (!new_result) { free(result); free(var_name); return NULL; }
                            result = new_result;
                        }
                        strcpy(result + result_len, var_value);
                        result_len += value_len;
                    }
                    free(var_name);
                }
                continue;
            } else {
                // Just a $, keep it
                if (result_len + 1 + 1 > bufsize) {
                    bufsize *= 2;
                    char *new_result = realloc(result, bufsize);
                    if (!new_result) { free(result); return NULL; }
                    result = new_result;
                }
                result[result_len++] = '$';
                result[result_len] = '\0';
                continue;
            }
        } else {
            // Regular character
            if (result_len + 1 + 1 > bufsize) {
                bufsize *= 2;
                char *new_result = realloc(result, bufsize);
                if (!new_result) { free(result); return NULL; }
                result = new_result;
            }
            result[result_len++] = *p;
            result[result_len] = '\0';
        }
        p++;
    }
    return result;
}

/**
 * expand_env_vars - Expand environment variables in all command arguments.
 * @cmd: Command structure to process.
 */
void expand_env_vars(command_t *cmd) {
    if (!cmd || !cmd->args) return;
    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->args[i] && strchr(cmd->args[i], '$')) { // Only expand if '$' present
            char *expanded = expand_env_var_in_string(cmd->args[i]);
            if (expanded) {
                free(cmd->args[i]);
                cmd->args[i] = expanded;
            }
        }
    }
}
