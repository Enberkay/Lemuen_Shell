#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Safe string duplication with error handling
char *strdup_safe(const char *str);

// String trimming functions
char *trim_left(char *str);
char *trim_right(char *str);
char *trim(char *str);

// String splitting
char **split_string(const char *str, const char *delim, int *count);
void free_string_array(char **array);

// Environment variable utilities
char *get_env_var(const char *name);
void set_env_var(const char *name, const char *value);

// Path utilities
char *expand_tilde(const char *path);
char *get_current_dir(void);

// Error handling
void print_error(const char *format, ...);
void print_system_error(const char *message);

#endif // UTILS_H
