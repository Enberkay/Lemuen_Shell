# Lemuen Shell v0.6

**A lightweight shell implementation written in C, designed for educational purposes and understanding shell internals.**

## Overview

Lemuen Shell is a command-line interpreter that provides a subset of POSIX shell functionality. It serves as both a learning tool for understanding shell architecture and a functional command-line environment.

## Current Features (v0.6)

### Core Functionality
- **Interactive Shell**: Command-line interface with colored prompt
- **Command History**: Navigable history using arrow keys (readline integration)
- **Builtin Commands**: `cd`, `exit`, `pwd`, `echo`, `help`, `export`, `unset`
- **External Command Execution**: Support for system commands (ls, cat, rm, etc.)
- **I/O Redirection**: Input (`<`), output (`>`, `>>`) redirection
- **Background Execution**: Process execution with `&` operator
- **Command Chaining**: Sequential execution with `;` separator
- **Signal Handling**: Proper handling of SIGINT (Ctrl+C)
- **Memory Management**: Leak-free implementation with proper cleanup

### Architecture Components
- **Parser**: Tokenization and command structure parsing
- **Executor**: Process creation and command execution
- **Builtins**: Internal command implementations
- **Utilities**: String manipulation and environment variable handling

## Installation and Usage

### Build Instructions
```bash
# Compile the shell
make

# Run the shell
./bin/lemuen

# Alternative: build and run in one command
make run
```

### Basic Commands
```bash
lemuen> ls                    # List directory contents
lemuen> pwd                   # Print working directory
lemuen> cd ~/Documents       # Change directory
lemuen> echo "Hello World"   # Print text
lemuen> help                 # Display builtin commands
lemuen> exit                 # Exit shell
```

### I/O Redirection Examples
```bash
lemuen> echo "Hello" > file.txt    # Write to file (overwrite)
lemuen> cat file.txt               # Read file contents
lemuen> echo "More" >> file.txt    # Append to file
lemuen> cat < file.txt             # Input redirection
```

### Process Control
```bash
lemuen> sleep 10 &           # Execute in background
lemuen> ls; pwd; echo done   # Sequential command execution
```

## Project Structure

```
Lemuen_Shell/
├── include/           # Header files
│   ├── builtins.h     # Builtin command declarations
│   ├── executor.h     # Command execution interface
│   ├── parser.h       # Command parsing interface
│   └── utils.h        # Utility function declarations
├── src/              # Source files
│   ├── main.c        # Main shell loop and signal handling
│   ├── builtins.c    # Builtin command implementations
│   ├── executor.c    # Command execution logic
│   ├── parser.c      # Command parsing implementation
│   └── utils.c       # Utility functions
├── obj/              # Object files (generated)
├── bin/              # Executable (generated)
├── Makefile          # Build configuration
└── README.md         # Documentation
```

## Technical Architecture

### 1. Main Loop (main.c)
```c
// Primary execution loop
while (readline()) {
    cmd = parse_command(line);
    if (cmd) {
        if (has_redirection) {
            execute_command(cmd);  // Fork+exec for redirection
        } else if (is_builtin) {
            run_builtin(cmd);      // Direct execution
        } else {
            execute_command(cmd);  // Fork+exec for external
        }
    }
}
```

### 2. Command Parsing (parser.c)
```c
// Parse: "echo hello > file.txt"
command_t {
    args: ["echo", "hello"]
    output_redirect: "file.txt"
    append_output: false
}
```

### 3. Command Execution (executor.c)
```c
// Execution strategy:
// - Builtin commands: direct execution in parent process
// - External commands: fork+exec in child process
// - Redirection: always fork+exec for proper file descriptor handling
```

### 4. Builtin Commands (builtins.c)
```c
// Internal commands executed without process creation
cd, pwd, echo, help, exit, export, unset
```

### 5. Utilities (utils.c)
```c
// String manipulation, environment variable management, path expansion
```

## Development and Debugging

### Build Targets
```bash
make help          # Display all available targets
make clean         # Remove build artifacts
make debug         # Build with debug symbols
make release       # Optimized release build
make valgrind      # Run with memory leak detection
```

### Memory Management
```bash
# Check for memory leaks
make valgrind
```

### Code Quality
```bash
make format        # Format code with clang-format
make cppcheck      # Run static analysis
```

## Implementation Details

### Process Management Strategy
- **Builtin Commands**: Execute directly in parent process for efficiency
- **External Commands**: Fork child process and execute with execvp()
- **Redirection**: Always fork to handle file descriptor manipulation
- **Background Execution**: Fork without waiting for completion

### Signal Handling
- **SIGINT (Ctrl+C)**: Properly handled with readline integration
- **Child Processes**: Signal handlers reset to default behavior
- **Parent Process**: Maintains shell state during signal events

### Memory Management
- **Command Structures**: Proper allocation and deallocation
- **String Arrays**: Null-terminated arrays with correct sizing
- **File Descriptors**: Proper cleanup after redirection operations

## Known Issues and Solutions

### 1. Redirection with Builtins
**Issue**: Builtin commands with redirection not working
**Root Cause**: Builtin check performed before redirection check
**Solution**: Check for redirection before builtin classification

### 2. Memory Management
**Issue**: Invalid pointer errors during cleanup
**Root Cause**: String arrays not properly null-terminated
**Solution**: Ensure null-termination in string splitting functions

### 3. Output Buffering
**Issue**: Builtin output not appearing with redirection
**Root Cause**: printf() buffering in child processes
**Solution**: Use write() or fflush() before process termination

## Roadmap

### Version 0.7
- Logical operators (`&&`, `||`)
- Environment variable expansion (`$VAR`)
- Enhanced error handling

### Version 0.8
- Globbing support (`*`, `?`, `[]`)
- Enhanced path expansion
- Improved command completion

### Version 0.9
- Subshell support (`$(command)`)
- Command aliases
- Configuration file support

### Version 1.0
- Job control (`jobs`, `fg`, `bg`)
- Persistent command history
- Advanced signal handling

## Technical Notes

### Dependencies
- **GNU Readline**: Command history and line editing
- **POSIX C**: Standard C library functions
- **GCC**: C compiler with C99 standard

### Build Requirements
- GCC compiler
- GNU Make
- Readline development libraries
- POSIX-compliant system

### Performance Characteristics
- **Memory Usage**: Minimal overhead for builtin commands
- **Process Creation**: Standard fork+exec for external commands
- **Response Time**: Immediate for builtins, system-dependent for externals

## Contributing

This project is designed for educational purposes. Contributions should focus on:
- Code clarity and documentation
- Educational value and learning opportunities
- POSIX compliance where applicable
- Memory safety and error handling

## License

This project is provided as-is for educational purposes. Use at your own discretion.

---

**Lemuen Shell v0.6** - A functional shell implementation demonstrating core shell concepts and system programming principles. 