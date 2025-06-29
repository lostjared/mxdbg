# mxdbg - Modern Debugger with AI Integration

A modern C++ debugger built with ptrace that integrates with Ollama for AI-powered code analysis and explanation.

![image](https://github.com/user-attachments/assets/149d94fd-6cdd-48f5-9171-ad83a066f26d)
![backtrace0x2](https://github.com/user-attachments/assets/6316ef35-e5d0-49b5-8465-148794326468)


## Color Key
- Cyan for AI
- Red for machine code/disassembly
- White for debugger text


#  Not complete, some features still need to be implemented. This is a new project.

## Features

- **Process Control**: Launch or attach to processes with full debugging capabilities
- **Register Manipulation**: Read and write 64-bit, 32-bit, 16-bit, and 8-bit registers
- **Memory Operations**: Read and write process memory with byte-level precision
- **Breakpoint Management**: Set, remove, and handle breakpoints
- **Single Stepping**: Execute instructions one at a time with full control
- **Disassembly**: View disassembled code using objdump integration
- **AI Integration**: Get AI-powered explanations of disassembly and code behavior
- **Interactive Shell**: Readline-based command interface with history

## Dependencies

### Required Dependencies

- **CMake 3.20+**: Build system
- **C++20 compatible compiler**: GCC 10+ or Clang 10+
- **readline**: For interactive command line interface
- **ollama_gen**: AI integration library for Ollama communication
- **Standard Linux tools**: objdump, ptrace support

### Installing ollama_gen

The project requires the `ollama_gen` library for AI integration. Install it by building from source. https://github.com/lostjared/ollama_gen


## Environment Setup

### Required Environment Variables

To use the AI integration features, you must export these environment variables to use with ollama

```bash
export MXDBG_HOST="localhost"     # Your Ollama server URL can included port
                                  # example 192.168.1.50:1088 op just the ip 192.168.1.50 
export MXDBG_MODEL="llama2"       # Your preferred Ollama model ex: codellama:7b
```

### Setting up Ollama

1. Install Ollama on your system
2. Start the Ollama service: `ollama serve`
3. Pull a model: `ollama pull llama2` (or your preferred model)
4. Set the environment variables as shown above

## Building

```bash
mkdir build
cd build
cmake ..
make
```

### Installation

```bash
sudo make install
```

### Uninstall

```bash
sudo make uninstall
```

## Usage

### Basic Usage

```bash
# Launch a program for debugging
./mxdbg /path/to/program

# Attach to an existing process
./mxdbg -p <PID>

# Dump assembly of a binary
./mxdbg -d /path/to/binary
```

### Command Line Options

- `-p <PID>`: Attach to process with given PID
- `-P <PID>`: Same as `-p` (long form)
- `-R <path>`: Launch executable at path
- `-r <path>`: Same as `-R` (short form)
- `-A <args>`: Additional arguments for the launched process
- `-a <args>`: Same as `-A` (short form)
- `-d`: Dump assembly of executable
- `-D`: Same as `-d` (long form)

### Interactive Commands

Once in the debugger shell (`mx $>`), you can use:

## Available Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `continue` | `c` | Continue process execution |
| `cur` | | Print current instruction |
| `step` | `s` | Execute single instruction |
| `step N` | `s N` | Execute N instructions |
| `status` | `st` | Show process status |
| `list` | `list_less` | Display disassembly |
| `registers` | `regs` | Show all registers |
| `register 8/16/32 <name>` | | Show specific register value |
| `set <reg> <value>` | | Set register to value |
| `break <addr>` | `b` | Set breakpoint at address |
| `watch <addr> <size> [type]` | | Set watchpoint (type: read/write/access) |
| `unwatch <addr>` | | Remove watchpoint at address |
| `watchpoints` | | List watch points |
| `read <addr>` | | Read memory at address |
| `read_bytes <address> <size>` | | Read bytes at address |
| `write <addr> <value>` | | Write value to memory at address |
| `write_bytes <ad> <va>` | | Write bytes to address |
| `local <reg> <ad> <size>` | | Local variable on stack |
| `explain <function>` | | Explain function disassembly with AI |
| `ask <question>` | | Ask the AI a question about the program |
| `user mode` | | User difficulty level for use with AI |
| `help` | `h` | Show this help message |
| `quit` | `q`, `exit` | Exit debugger |

## AI Integration

When `MXDBG_HOST` and `MXDBG_MODEL` are set, the debugger will:

- Provide AI explanations when stepping through code
- Analyze disassembly output with the `explain` command
- Offer context-aware debugging assistance with the ask command

Example AI integration:
```bash
mx $> explain function
# AI will analyze the program's disassembly and explain its behavior
```

## Project Structure

```
mxdbg/
├── libmxdbg/           # Core debugger library
│   ├── include/mxdbg/  # Public headers
│   ├── process.cpp     # Process control and ptrace operations
│   ├── debugger.cpp    # Main debugger logic
│   └── pipe.cpp        # IPC utilities
├── mxdbg/              # Main executable
│   └── main.cpp        # Entry point and argument parsing
└── tests/              # Test suite
    ├── process_*.cpp   # Process control tests
    ├── pipe_*.cpp      # IPC tests
    └── register_*.cpp  # Register manipulation tests
```

## Supported Architectures

- x86_64 Linux systems
- Requires ptrace support in kernel

## Register Support

### 64-bit Registers
`rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rbp`, `rsp`, `rip`, `r8`-`r15`

### 32-bit Registers  
`eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp`, `esp`, `r8d`-`r15d`

### 16-bit Registers
`ax`, `bx`, `cx`, `dx`, `si`, `di`, `bp`, `sp`, `r8w`-`r15w`

### 8-bit Registers
`al`, `ah`, `bl`, `bh`, `cl`, `ch`, `dl`, `dh`, `sil`, `dil`, `bpl`, `spl`, `r8b`-`r15b`

## Testing

Run the test suite:
```bash
cd build
make test
```

Individual tests can be run:
```bash
./tests/process_continue_test
./tests/pipe_test
./tests/register_test
```

**Build errors**: Ensure all dependencies are installed, especially ollama_gen and readline development packages.
