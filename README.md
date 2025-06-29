# 🐚 Lemuen Shell v0.6

**A lightweight shell written in C, designed for learning and understanding shell internals.**

## 📋 สรุปสั้นๆ สำหรับตัวเอง

### 🎯 **ตอนนี้ทำอะไรได้แล้ว (v0.6)**
- ✅ **Interactive shell** พร้อม prompt สี
- ✅ **Command history** (ลูกศรขึ้น/ลง)
- ✅ **Builtin commands**: `cd`, `exit`, `pwd`, `echo`, `help`, `export`, `unset`
- ✅ **External commands** (ls, cat, rm, etc.)
- ✅ **Redirection**: `>`, `>>`, `<`
- ✅ **Background execution**: `&`
- ✅ **Command chaining**: `;`
- ✅ **Signal handling**: Ctrl+C
- ✅ **Memory management**: ไม่มี leaks

### 🚀 **ต่อไปจะทำอะไร (Roadmap)**
- **v0.7**: `&&`, `||`, `$VAR` expansion
- **v0.8**: Globbing (`*`, `?`, `[]`)
- **v0.9**: Subshells, aliases
- **v1.0**: Job control, history file

---

## 🛠️ การใช้งาน

### Build และ Run
```bash
# Build
make

# Run
./bin/lemuen

# หรือ
make run
```

### คำสั่งที่ใช้บ่อย
```bash
lemuen> ls                    # List files
lemuen> pwd                   # Current directory
lemuen> cd ~/Documents       # Change directory
lemuen> echo "Hello"         # Print text
lemuen> help                 # Show builtins
lemuen> exit                 # Exit shell
```

### Redirection (สำคัญ!)
```bash
lemuen> echo "Hello" > file.txt    # Write to file
lemuen> cat file.txt               # Read file
lemuen> echo "More" >> file.txt    # Append to file
lemuen> cat < file.txt             # Input redirection
```

### Background & Chaining
```bash
lemuen> sleep 10 &           # Run in background
lemuen> ls; pwd; echo done   # Multiple commands
```

---

## 🏗️ โครงสร้างโปรเจกต์

```
lemuen_shell/
├── include/           # Header files
│   ├── builtins.h     # Builtin command definitions
│   ├── executor.h     # Command execution
│   ├── parser.h       # Command parsing
│   └── utils.h        # Utility functions
├── src/              # Source files
│   ├── main.c        # Main shell loop + signal handling
│   ├── builtins.c    # Builtin implementations
│   ├── executor.c    # Command execution logic
│   ├── parser.c      # Command parsing logic
│   └── utils.c       # Utility functions
├── obj/              # Object files (generated)
├── bin/              # Executable (generated)
├── Makefile          # Build configuration
└── README.md         # This file
```

---

## 🔧 Architecture (สำหรับตัวเอง)

### 1. **main.c** - จุดเริ่มต้น
```c
// Main loop
while (readline()) {
    cmd = parse_command(line);
    if (cmd) {
        if (has_redirection) {
            execute_command(cmd);  // ใช้ fork+exec
        } else if (is_builtin) {
            run_builtin(cmd);      // เรียกตรง
        } else {
            execute_command(cmd);  // ใช้ fork+exec
        }
    }
}
```

### 2. **parser.c** - แยกคำสั่ง
```c
// Parse: "echo hello > file.txt"
command_t {
    args: ["echo", "hello"]
    output_redirect: "file.txt"
    append_output: false
}
```

### 3. **executor.c** - รันคำสั่ง
```c
// ถ้ามี redirection -> fork+exec
// ถ้าไม่มี redirection -> เรียกตรง (builtin) หรือ fork+exec (external)
```

### 4. **builtins.c** - คำสั่งในตัว
```c
// Builtin commands ที่ไม่ต้อง fork
cd, pwd, echo, help, exit, export, unset
```

### 5. **utils.c** - ฟังก์ชันช่วยเหลือ
```c
// String manipulation, environment variables, path expansion
```

---

## 🐛 Debugging

### Memory Leaks
```bash
make valgrind
```

### Build Options
```bash
make help          # Show all targets
make clean         # Clean build
make debug         # Debug build
make release       # Optimized build
```

---

## 💡 สิ่งที่เรียนรู้

### 1. **Redirection Logic**
- **ปัญหาเดิม**: builtin + redirection ไม่ทำงาน
- **สาเหตุ**: main.c เช็ค `is_builtin()` ก่อนเช็ค redirection
- **วิธีแก้**: เช็ค redirection ก่อน builtin

### 2. **Memory Management**
- **ปัญหาเดิม**: `free(): invalid pointer`
- **สาเหตุ**: `split_string()` ไม่ได้ null-terminate array
- **วิธีแก้**: เพิ่ม `tokens[size] = NULL;`

### 3. **Process Management**
- **Builtin**: รันใน parent process
- **External + Redirection**: fork+exec ใน child process
- **Background**: fork แล้วไม่รอ

### 4. **Signal Handling**
- **Ctrl+C**: `handle_sigint()` -> `rl_on_new_line()`
- **Child processes**: reset signal handlers

---

## 🔍 ปัญหาที่เจอและวิธีแก้

### 1. **Redirection ไม่ทำงาน**
```
ปัญหา: echo test > file.txt ไม่สร้างไฟล์
สาเหตุ: main.c เช็ค builtin ก่อน redirection
วิธีแก้: เช็ค redirection ก่อน builtin ใน main.c
```

### 2. **Memory Leaks**
```
ปัญหา: free(): invalid pointer
สาเหตุ: Array ไม่ได้ null-terminated
วิธีแก้: เพิ่ม null-terminator ใน split_string()
```

### 3. **Builtin ไม่ทำงานกับ redirection**
```
ปัญหา: pwd > file.txt ไม่ทำงาน
สาเหตุ: ใช้ printf() ใน child process
วิธีแก้: ใช้ write() หรือ fflush() ก่อน exit
```

---

## 📝 Notes สำหรับตัวเอง

### **สิ่งที่ทำได้แล้ว**
- ✅ Shell พื้นฐานทำงานได้
- ✅ Redirection ครบถ้วน
- ✅ Memory management ดี
- ✅ Error handling ครบ

### **สิ่งที่ต้องทำต่อ**
- [ ] Logical operators (`&&`, `||`)
- [ ] Environment variables (`$HOME`, `$PATH`)
- [ ] Globbing (`ls *.txt`)
- [ ] Job control (`jobs`, `fg`, `bg`)
- [ ] History file
- [ ] Configuration file

### **คำสั่งที่ใช้บ่อย**
```bash
make clean && make    # Rebuild
make valgrind         # Check memory
./bin/lemuen          # Run shell
```

---

## 🎯 สรุป

**Lemuen Shell v0.6** เป็น shell ที่ทำงานได้จริง มีฟีเจอร์ครบถ้วนสำหรับการใช้งานพื้นฐาน และพร้อมสำหรับการพัฒนาต่อไป

**Key Features:**
- Interactive shell with history
- Builtin + external commands
- Redirection (>, >>, <)
- Background execution
- Command chaining
- Signal handling
- Memory safe

**Next Steps:**
- Add logical operators
- Add environment variable expansion
- Add globbing support
- Add job control

---

**Happy shelling! 🐚✨** 