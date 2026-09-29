# mxdbg - ELF x86_64 Debugger with AI Integration

coded by Jared Bruni (jaredbruni@protonmail.com)

A modern C++ debugger built with ptrace that integrates with Ollama, OpenAI, or Anthropic for AI-powered code analysis and explanation.

![image](https://github.com/user-attachments/assets/149d94fd-6cdd-48f5-9171-ad83a066f26d)
![backtrace0x2](https://github.com/user-attachments/assets/6316ef35-e5d0-49b5-8465-148794326468)


## Color Key
- Cyan for AI
- Red for machine code/disassembly
- White for debugger text

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

- **CMake 3.20+ or Pcons**: Build system
- **C++20 compatible compiler**: GCC 10+ or Clang 10+
- **readline**: For interactive command line interface
- **ollama_gen**: AI integration library for Ollama, OpenAI, and Anthropic communication
- **Standard Linux tools**: objdump, ptrace support

### Installing ollama_gen

The project requires the `ollama_gen` library for AI integration. Install it by building from source. https://github.com/lostjared/ollama_gen


## Connecting an AI Provider

mxdbg reads its AI configuration from environment variables. Set
`MXDBG_PROVIDER` to `ollama`, `openai`, or `anthropic`, and set
`MXDBG_MODEL` to a model identifier supported by that provider.
`MXDBG_PROVIDER` defaults to `ollama` for backward compatibility.

Environment variables set with `export` apply to the current shell and programs
started from it. Add them to your shell profile if you want them available in
future terminal sessions. Do not commit API keys to this repository.

### Connect to Ollama

Start Ollama and make sure the selected model is installed:

```bash
ollama serve
ollama pull llama2
```

In another terminal, configure mxdbg:

```bash
export MXDBG_PROVIDER="ollama"
export MXDBG_HOST="localhost"     # Optional; defaults to localhost
export MXDBG_MODEL="llama2"
```

`MXDBG_HOST` may include a port, for example `192.168.1.50:11434`.

### Connect to OpenAI

Create an API key in the OpenAI dashboard, then export it as
`OPENAI_API_KEY` along with the provider and model:

```bash
export MXDBG_PROVIDER="openai"
export MXDBG_MODEL="your-openai-model"
export OPENAI_API_KEY="your_openai_api_key"
```

mxdbg reads `OPENAI_API_KEY` automatically and does not accept the key as a
command-line argument.

### Connect to Anthropic

Create an Anthropic API key, then configure mxdbg with:

```bash
export MXDBG_PROVIDER="anthropic"
export MXDBG_MODEL="your-anthropic-model"
export ANTHROPIC_API_KEY="your_anthropic_api_key"
```

mxdbg reads `ANTHROPIC_API_KEY` automatically and does not accept the key as a
command-line argument.

### Optional settings

The following settings apply when needed:

```bash
# Override the default OpenAI or Anthropic base URL for a proxy or compatible API.
export MXDBG_BASE_URL="https://your-proxy.example"

# Set the bounded AI debugging-context budget (4 KiB to 1 MiB).
export MXDBG_CONTEXT_SIZE="32768"
```

Leave `MXDBG_BASE_URL` unset to use the provider's standard endpoint. It is not
used for Ollama; use `MXDBG_HOST` for Ollama instead.

### Start mxdbg and use the provider

Start mxdbg from the same shell where the variables were exported:

```bash
./build/mxdbg/mxdbg /path/to/program
```

Then invoke an AI feature from the debugger prompt:

```text
mx $> ask why did this function crash?
mx $> explain main
```

For Ollama, mxdbg checks the configured server during startup. For OpenAI and
Anthropic, startup only validates the configuration; the first provider request
is made when an AI feature such as `ask` or `explain` needs a response.

To disable AI integration for a run, use:

```bash
./build/mxdbg/mxdbg --disable-ai /path/to/program
```

If mxdbg reports that a variable is missing, verify its presence without
printing the secret itself:

```bash
test -n "$OPENAI_API_KEY" && echo "OPENAI_API_KEY is set"
test -n "$ANTHROPIC_API_KEY" && echo "ANTHROPIC_API_KEY is set"
```

## Building

The `mxdbg` and `ollama_gen` repositories may be checked out anywhere. Replace
the example paths below with the locations of the two independent checkouts.

### Pcons

Install [uv](https://docs.astral.sh/uv/), then build `mxdbg` directly against
an `ollama_gen` source checkout:

```bash
cd /path/to/mxdbg
uvx pcons -B build/pcons \
    OLLAMA_GEN_SOURCE_DIR=/path/to/ollama_gen \
    --reconfigure
```

The executable is written to `build/pcons/mxdbg`. Run the tests with:

```bash
uvx pcons -B build/pcons test
```

Tests that launch or attach to processes require permission to use `ptrace`.

If `ollama_gen` has already been installed in a standard prefix, omit
`OLLAMA_GEN_SOURCE_DIR`. For a non-standard installation, pass its prefix:

```bash
uvx pcons -B build/pcons PREFIX=/path/to/ollama-prefix --reconfigure
```

To stage an installation under the `mxdbg` repository's `dist` directory:

```bash
uvx pcons -B build/pcons all install
```

Use `PCONS_INSTALL_PREFIX=/path/to/prefix` to select another installation
prefix, `VARIANT=debug` for a debug build, or `TESTS=0` to omit the test
programs.

### CMake

```bash
cmake -S . -B build/cmake \
    -DOLLAMA_GEN_SOURCE_DIR=/path/to/ollama_gen
cmake --build build/cmake
```

### Installation

```bash
sudo cmake --install build/cmake
```

### Uninstall

```bash
sudo cmake --build build/cmake --target uninstall
```

## Usage

### Basic Usage

```bash
# Show command-line help, including AI provider setup
./mxdbg -h

# Show version information
./mxdbg -v

# Launch a program for debugging
./mxdbg /path/to/program

# Attach to an existing process
./mxdbg -p <PID>

# Dump assembly of a binary
./mxdbg -d /path/to/binary
```

### Command Line Options

- `-h`, `--help`: Show usage and local/remote AI provider setup
- `-v`, `--version`: Show version information
- `-p <PID>`, `--pid <PID>`: Attach to a process
- `-r <path>`, `--path <path>`: Launch an executable
- `-a <args>`, `--args <args>`: Pass arguments to the launched process
- `-d <path>`, `--dump <path>`: Dump executable assembly
- `--disable-ai`: Run without AI integration

### Interactive Commands

Once in the debugger shell (`mx $>`), you can use:

## Available Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| **Expression & Variables** | | |
| `expr <e>` | | Evaluate expression |
| `setval <name> <value>` | | Set variable to value |
| `listval` | | List variables |
| **Process Control** | | |
| `run` | `r` | Run program (sets main breakpoint) |
| `continue` | `c` | Continue process execution |
| `step` | `s` | Execute single instruction |
| `step N` | `s N` | Execute N instructions |
| `next` | | Step over function calls |
| `finish` | `step_out` | Step out of current function |
| `until <addr>` | `run_until <addr>` | Run until specific address |
| `status` | `st` | Show process status |
| `start` | `restart` | Restart the program |
| **Threading** | | |
| `thread` | | Show current thread |
| `thread <id>` | | Switch to thread context |
| `threads` | | List all running threads |
| `debug_thread <id>` | | Debug specific thread |
| **Code Analysis** | | |
| `cur` | `current` | Print current instruction |
| `list` | | Display full disassembly |
| `list_less` | | Display disassembly with pager |
| `list_function <name>` | | Show specific function disassembly |
| `base` | | Show base address and current PC |
| `backtrace` | `bt`, `where` | Show call stack backtrace |
| **Registers** | | |
| `registers` | `regs` | Show all registers |
| `register <name>` | `reg <name>` | Show specific register value |
| `register32 <name>` | | Show 32-bit register |
| `register16 <name>` | | Show 16-bit register |
| `register8 <name>` | | Show 8-bit register |
| `set <reg> <value>` | | Set register to value |
| **FPU/Float Registers** | | |
| `get_fpu <name>` | | Get FPU register value |
| `set_fpu <name> <value>` | | Set FPU register to value |
| `list_fpu` | | List all FPU registers |
| **Breakpoints & Watchpoints** | | |
| `break <addr>` | `b <addr>` | Set breakpoint at address |
| `break_if <addr> <condition>` | | Set conditional breakpoint that only triggers when condition is true |
| `function <name>` | | Set breakpoint at function |
| `list_break` | `lb` | List all breakpoints |
| `remove <addr/index>` | `rmv` | Remove breakpoint |
| `watch <addr> <size> [type]` | | Set watchpoint (type: read/write/access) |
| `watchpoints` | `wp` | List watchpoints |
| **Memory Operations** | | |
| `read <addr>` | | Read 8 bytes from memory address |
| `read_bytes <addr> <size>` | | Read specific number of bytes |
| `write <addr> <value>` | | Write value to memory address |
| `write_bytes <addr> <bytes>` | | Write byte sequence to memory |
| `hexdump <addr> <size>` | | Display memory as hexadecimal dump |
| `as_bytes <value>` | | Convert value to byte representation |
| `maps` | `memory_maps` | Show memory map |
| `local <reg> <offset> <size>` | | Read local variable on stack |
| **Memory Search** | | |
| `search int <value>` | | Search for 32-bit integer in memory |
| `search int64 <value>` | | Search for 64-bit integer in memory |
| `search string <text>` | | Search for string in memory |
| `search bytes <hex bytes>` | | Search for byte pattern |
| `search pattern <pattern>` | | Search with wildcards (e.g., 41??43) |
| **Stack Analysis** | | |
| `stack_frame` | | Analyze current stack frame |
| **AI Features** | | |
| `explain <function>` | | Explain function disassembly with AI |
| `ask <question>` | | Ask the AI a question about the program |
| `context` | | Show bounded debugging context, including the latest crash snapshot |
| `context clear` | | Clear the AI debugging context |
| `mode <level>` | `user <level>` | Set AI difficulty (beginner/programmer/expert) |
| **File Information** | | |
| `info files` | | Show open file descriptors |
| **Utility** | | |
| `find <text>` | | Find text in disassembly using grep |
| `shell <command>` | `sh <command>` | Execute shell command |
| `clear` | | Clear the screen |
| `debug_state` | | Show detailed debug state |
| `help` | `h` | Show this help message |
| `quit` | `q`, `exit` | Exit debugger |

## AI Integration

When `MXDBG_MODEL` and the selected provider's required variables are set, the debugger will:

- Provide AI explanations when stepping through code
- Analyze disassembly output with the `explain` command
- Offer context-aware debugging assistance with the ask command
- Preserve fatal-signal details, registers, the faulting instruction, stack data,
  backtrace frames, and relevant mappings for the `context` and `ask` commands

The `explain` command performs isolated static function analysis and does not
inherit runtime crash state.

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
cd build/tests
ctest
```

Individual tests can be run:
```bash
./tests/process_continue_test
./tests/pipe_test
./tests/register_test
```

**Build errors**: Ensure all dependencies are installed, especially ollama_gen and readline development packages.
