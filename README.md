# 🐚 Lemuen Shell v0.5

A lightweight shell written in C, designed for learning and understanding shell internals.

## ✨ Features

### 🎯 Current Features (v0.5)
- **Interactive shell** with colored prompt
- **Command history** using readline
- **Builtin commands**: `cd`, `exit`, `pwd`, `echo`, `help`, `export`, `unset`
- **External command execution** with PATH search
- **Input/Output redirection** (`<`, `>`, `>>`)
- **Background execution** (`&`)
- **Command chaining** (`;`)
- **Tilde expansion** (`~`)
- **Signal handling** (Ctrl+C)
- **Error handling** with colored error messages

### 🚀 Planned Features
- **v0.6**: Logical operators (`&&`, `||`), environment variables
- **v0.7**: Globbing and wildcards (`*`, `?`, `[]`)
- **v0.8**: Subshells and command substitution
- **v0.9**: Configuration files and aliases
- **v1.0**: Job control, advanced signal handling, history file

## 🛠️ Building

### Prerequisites
```bash
# On Arch Linux
sudo pacman -S gcc make readline

# On Ubuntu/Debian
sudo apt install gcc make libreadline-dev

# On macOS
brew install readline
```

### Build and Run
```bash
# Build the shell
make

# Run the shell
make run

# Or run directly
./bin/lemuen
```

### Other Make Targets
```bash
make help          # Show all available targets
make clean         # Clean build artifacts
make debug         # Build with debug flags
make release       # Build optimized version
make valgrind      # Run with memory leak detection
make install       # Install to /usr/local/bin
```

## 📖 Usage

### Basic Commands
```bash
lemuen> ls -la                    # List files
lemuen> pwd                       # Print working directory
lemuen> cd ~/Documents           # Change directory
lemuen> echo "Hello, World!"     # Print text
lemuen> help                     # Show builtin commands
lemuen> exit                     # Exit shell
```

### Redirection
```bash
lemuen> ls > files.txt           # Redirect output to file
lemuen> cat < input.txt          # Redirect input from file
lemuen> echo "append" >> log.txt # Append to file
```

### Background Execution
```bash
lemuen> sleep 10 &               # Run command in background
lemuen> [1234] sleep
```

### Command Chaining
```bash
lemuen> cd /tmp; ls; pwd         # Execute multiple commands
```

### Environment Variables
```bash
lemuen> export PATH=/usr/bin:/bin
lemuen> export MY_VAR="hello"
lemuen> unset MY_VAR
```

## 🏗️ Project Structure

```
lemuen_shell/
├── include/           # Header files
│   ├── builtins.h     # Builtin command definitions
│   ├── executor.h     # Command execution
│   ├── parser.h       # Command parsing
│   └── utils.h        # Utility functions
├── src/              # Source files
│   ├── main.c        # Main shell loop
│   ├── builtins.c    # Builtin implementations
│   ├── executor.c    # Command execution logic
│   ├── parser.c      # Command parsing logic
│   └── utils.c       # Utility functions
├── obj/              # Object files (generated)
├── bin/              # Executable (generated)
├── Makefile          # Build configuration
└── README.md         # This file
```

## 🔧 Architecture

### Core Components

1. **Parser** (`parser.c`)
   - Parses command line input
   - Handles redirections, background execution, command chaining
   - Returns structured `command_t` objects

2. **Executor** (`executor.c`)
   - Executes commands (builtin and external)
   - Handles process creation and management
   - Manages file redirections

3. **Builtins** (`builtins.c`)
   - Implements shell builtin commands
   - Extensible builtin command system

4. **Utils** (`utils.c`)
   - String manipulation utilities
   - Environment variable handling
   - Path expansion and validation

### Key Data Structures

```c
typedef struct {
    char **args;           // Command arguments
    int argc;              // Argument count
    char *input_redirect;  // Input redirection file
    char *output_redirect; // Output redirection file
    int append_output;     // Append mode for output
    char *next_command;    // Next command in chain
    int background;        // Background execution flag
} command_t;
```

## 🐛 Debugging

### Memory Leaks
```bash
make valgrind
```

### Static Analysis
```bash
make cppcheck
```

### Debug Build
```bash
make debug
```