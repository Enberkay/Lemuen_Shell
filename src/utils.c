#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

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
