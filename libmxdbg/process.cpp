#include"mxdbg/process.hpp"
#include"mxdbg/pipe.hpp"
#include"mxdbg/exception.hpp"
#include<sys/ptrace.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdexcept>
#include<cstring>
#include<sstream>
#include<iostream>
#include<vector>
#include<thread>
#include<chrono>
#include<fstream>
#include<iomanip>

namespace mx {
    
    Process::Process(Process&& proc) : m_pid(proc.m_pid) {
        proc.m_pid = -1; 
    }
   
    Process &Process::operator=(Process&& other) {
        if (this != &other) {
            m_pid = other.m_pid;
            other.m_pid = -1; 
        }
        return *this;
    }

    std::unique_ptr<Process> Process::launch(const std::filesystem::path& program, const std::vector<std::string> &args) {

        if (!std::filesystem::exists(program)) {
            throw mx::Exception("Executable file does not exist: " + program.string());
        }
        if (!std::filesystem::is_regular_file(program)) {
            throw mx::Exception("Path is not a regular file: " + program.string());
        }
        if (access(program.c_str(), X_OK) != 0) {
            throw mx::Exception("Executable file is not accessible: " + program.string());
        }
        
        mx::Pipe error_pipe;
        pid_t pid = fork();
        if (pid == 0) {
            error_pipe.close_read();          
            if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
                std::string error_msg = "PTRACE_FAILED: " + std::string(strerror(errno));
                error_pipe.write(error_msg);
                error_pipe.close_write();
                exit(1);
            }
            std::vector<char*> args_vec;
            args_vec.push_back(const_cast<char*>(program.filename().c_str()));
            for (const auto& arg : args) {
                args_vec.push_back(const_cast<char*>(arg.c_str()));
            }
            args_vec.push_back(nullptr);
            error_pipe.close_write();
            execvp(program.string().c_str(), args_vec.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            error_pipe.close_write();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::string error_msg = error_pipe.read_nonblocking();
            if (!error_msg.empty()) {
                error_pipe.close();
                kill(pid, SIGKILL);
                int status;
                waitpid(pid, &status, 0);
                throw mx::Exception("Failed to launch process: " + error_msg);
            }
            error_pipe.close();
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                throw mx::Exception::error("Failed to wait for child process");
            }
            if (!WIFSTOPPED(status)) {
                throw mx::Exception("Child process did not stop as expected");
            }
            return std::unique_ptr<Process>(new Process(pid));
        } else {
            throw mx::Exception::error("Failed to fork");
        }
    }

    std::unique_ptr<Process> Process::attach(pid_t pid) {
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
            throw mx::Exception::error("Failed to attach to process");
        }
        int status;
        int result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            result = waitpid(pid, &status, 0);
        }
        
        if (result == -1) {
            ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
            throw mx::Exception::error("Failed to wait for attached process");
        }        
        return std::unique_ptr<Process>(new Process(pid));
    }

    void Process::wait_for_stop() {
        int status;
        if (waitpid(m_pid, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for process");
        }
        
        if (WIFEXITED(status)) {
            std::cout << "Process exited with code: " << WEXITSTATUS(status) << std::endl;
            return;
        } else if (WIFSIGNALED(status)) {
            std::cout << "Process killed by signal: " << format_signal(WTERMSIG(status)) << std::endl;
            return;
        } else if (WIFSTOPPED(status)) {
            int signal = WSTOPSIG(status);
            std::cout << "Process stopped by signal: " << format_signal(signal) << std::endl;
            
            if (signal == SIGTRAP) {
                uint64_t pc = get_pc();           
                if (breakpoints.find(pc) != breakpoints.end()) {
                    std::cout << "Breakpoint hit at " << format_hex64(pc) << std::dec << std::endl;
                    return;
                } else if (breakpoints.find(pc - 1) != breakpoints.end()) {
                    std::cout << "Breakpoint hit at " << format_hex64(pc - 1) << std::dec << std::endl;
                    set_pc(pc - 1);
                    return;
                } else {
                    
                }
            }
        }
    }

    void Process::continue_execution() {
        if (ptrace(PTRACE_CONT, m_pid, nullptr, 0) == -1) {
            throw mx::Exception::error("Failed to continue process");
        }
    }

    void Process::single_step() {
        uint64_t pc = get_pc();
        
        if (has_breakpoint(pc)) {
            handle_breakpoint_step(pc);
        } else {
            if (ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr) == -1) {
                throw mx::Exception::error("Failed to single step process");
            }
            is_single_stepping = true;
        }
    }

    void Process::handle_breakpoint_step(uint64_t address) {
        uint8_t original_byte = breakpoints[address];
        long data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        long restored = (data & ~0xFF) | original_byte;
        ptrace(PTRACE_POKEDATA, m_pid, address, restored);
        
        ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
        is_single_stepping = true;
        
    }

    void Process::wait_for_single_step() {
        int status;
        if (waitpid(m_pid, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for single step");
        }
        
        if (is_single_stepping) {
            uint64_t prev_pc = get_pc() - 1; 
            auto it = breakpoints.find(prev_pc);
            if (it != breakpoints.end()) {
                long data = ptrace(PTRACE_PEEKDATA, m_pid, prev_pc, nullptr);
                long data_with_int3 = (data & ~0xFF) | 0xCC;
                ptrace(PTRACE_POKEDATA, m_pid, prev_pc, data_with_int3);
            }
            is_single_stepping = false;
        }
    }

    void Process::set_pc(uint64_t address) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers");
        }
        regs.rip = address;
        if (ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    int Process::get_exit_status() {
        int status;
        pid_t result = waitpid(m_pid, &status, WNOHANG);
        if (result == m_pid) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return -WTERMSIG(status);  
            }
        }
        return -1; 
    }

    std::string Process::proc_info() const {
        std::ostringstream stream;
        std::filesystem::path proc_path = "/proc/" + std::to_string(m_pid);
        if (!std::filesystem::exists(proc_path)) {
            return "Process not found in procfs";
        }
        std::ifstream status_file(proc_path / "status");
        if (status_file.is_open()) {
            std::string line;
            while (std::getline(status_file, line)) {
                stream << line << "\n";
            }
            status_file.close();
        } else {
            stream << "Unable to read process status from " << proc_path.string() << "/status";
        }
        return stream.str();
    }

    std::string Process::reg_info() const {
        std::ostringstream stream;
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        stream << "RAX: " << std::hex << std::uppercase << regs.rax << "\n"
               << "RBX: " << regs.rbx << "\n"
               << "RCX: " << regs.rcx << "\n"
               << "RDX: " << regs.rdx << "\n"
               << "RSI: " << regs.rsi << "\n"
               << "RDI: " << regs.rdi << "\n"
               << "RBP: " << regs.rbp << "\n"
               << "RSP: " << regs.rsp << "\n"
               << "RIP: " << regs.rip
               << "\n"
               << "RFLAGS: " << regs.eflags << "\n"
               << "CS: " << regs.cs << "\n"
               << "SS: " << regs.ss << "\n"
               << "DS: " << regs.ds << "\n"
               << "ES: " << regs.es << "\n";

        stream << "\n";
        return stream.str();
    }


    uint64_t Process::get_register(const std::string &reg_name) const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "rax") return regs.rax;
        if (reg_name == "rbx") return regs.rbx;
        if (reg_name == "rcx") return regs.rcx;
        if (reg_name == "rdx") return regs.rdx;
        if (reg_name == "rsi") return regs.rsi;
        if (reg_name == "rdi") return regs.rdi;
        if (reg_name == "rbp") return regs.rbp;
        if (reg_name == "rsp") return regs.rsp;
        if (reg_name == "rip") return regs.rip;
        if (reg_name == "r8") return regs.r8;
        if (reg_name == "r9") return regs.r9;
        if (reg_name == "r10") return regs.r10;
        if (reg_name == "r11") return regs.r11;
        if (reg_name == "r12") return regs.r12;
        if (reg_name == "r13") return regs.r13;
        if (reg_name == "r14") return regs.r14;
        if (reg_name == "r15") return regs.r15;
        if (reg_name == "eflags") return regs.eflags;
        
        if (reg_name == "eax") return regs.rax & 0xFFFFFFFF;
        if (reg_name == "ebx") return regs.rbx & 0xFFFFFFFF;
        if (reg_name == "ecx") return regs.rcx & 0xFFFFFFFF;
        if (reg_name == "edx") return regs.rdx & 0xFFFFFFFF;
        if (reg_name == "esi") return regs.rsi & 0xFFFFFFFF;
        if (reg_name == "edi") return regs.rdi & 0xFFFFFFFF;
        if (reg_name == "ebp") return regs.rbp & 0xFFFFFFFF;
        if (reg_name == "esp") return regs.rsp & 0xFFFFFFFF;
        if (reg_name == "r8d") return regs.r8 & 0xFFFFFFFF;
        if (reg_name == "r9d") return regs.r9 & 0xFFFFFFFF;
        if (reg_name == "r10d") return regs.r10 & 0xFFFFFFFF;
        if (reg_name == "r11d") return regs.r11 & 0xFFFFFFFF;
        if (reg_name == "r12d") return regs.r12 & 0xFFFFFFFF;
        if (reg_name == "r13d") return regs.r13 & 0xFFFFFFFF;
        if (reg_name == "r14d") return regs.r14 & 0xFFFFFFFF;
        if (reg_name == "r15d") return regs.r15 & 0xFFFFFFFF;
        
        if (reg_name == "ax") return regs.rax & 0xFFFF;
        if (reg_name == "bx") return regs.rbx & 0xFFFF;
        if (reg_name == "cx") return regs.rcx & 0xFFFF;
        if (reg_name == "dx") return regs.rdx & 0xFFFF;
        if (reg_name == "si") return regs.rsi & 0xFFFF;
        if (reg_name == "di") return regs.rdi & 0xFFFF;
        if (reg_name == "bp") return regs.rbp & 0xFFFF;
        if (reg_name == "sp") return regs.rsp & 0xFFFF;
        if (reg_name == "r8w") return regs.r8 & 0xFFFF;
        if (reg_name == "r9w") return regs.r9 & 0xFFFF;
        if (reg_name == "r10w") return regs.r10 & 0xFFFF;
        if (reg_name == "r11w") return regs.r11 & 0xFFFF;
        if (reg_name == "r12w") return regs.r12 & 0xFFFF;
        if (reg_name == "r13w") return regs.r13 & 0xFFFF;
        if (reg_name == "r14w") return regs.r14 & 0xFFFF;
        if (reg_name == "r15w") return regs.r15 & 0xFFFF;
        
        if (reg_name == "al") return regs.rax & 0xFF;
        if (reg_name == "ah") return (regs.rax >> 8) & 0xFF;
        if (reg_name == "bl") return regs.rbx & 0xFF;
        if (reg_name == "bh") return (regs.rbx >> 8) & 0xFF;
        if (reg_name == "cl") return regs.rcx & 0xFF;
        if (reg_name == "ch") return (regs.rcx >> 8) & 0xFF;
        if (reg_name == "dl") return regs.rdx & 0xFF;
        if (reg_name == "dh") return (regs.rdx >> 8) & 0xFF;
        if (reg_name == "sil") return regs.rsi & 0xFF;
        if (reg_name == "dil") return regs.rdi & 0xFF;
        if (reg_name == "bpl") return regs.rbp & 0xFF;
        if (reg_name == "spl") return regs.rsp & 0xFF;
        if (reg_name == "r8b") return regs.r8 & 0xFF;
        if (reg_name == "r9b") return regs.r9 & 0xFF;
        if (reg_name == "r10b") return regs.r10 & 0xFF;
        if (reg_name == "r11b") return regs.r11 & 0xFF;
        if (reg_name == "r12b") return regs.r12 & 0xFF;
        if (reg_name == "r13b") return regs.r13 & 0xFF;
        if (reg_name == "r14b") return regs.r14 & 0xFF;
        if (reg_name == "r15b") return regs.r15 & 0xFF;
        
        throw mx::Exception("Unknown register name: " + reg_name);
    }

    uint32_t Process::get_register_32(const std::string &reg_name) const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "eax") return regs.rax & 0xFFFFFFFF;
        if (reg_name == "ebx") return regs.rbx & 0xFFFFFFFF;
        if (reg_name == "ecx") return regs.rcx & 0xFFFFFFFF;
        if (reg_name == "edx") return regs.rdx & 0xFFFFFFFF;
        if (reg_name == "esi") return regs.rsi & 0xFFFFFFFF;
        if (reg_name == "edi") return regs.rdi & 0xFFFFFFFF;
        if (reg_name == "ebp") return regs.rbp & 0xFFFFFFFF;
        if (reg_name == "esp") return regs.rsp & 0xFFFFFFFF;
        if (reg_name == "r8d") return regs.r8 & 0xFFFFFFFF;
        if (reg_name == "r9d") return regs.r9 & 0xFFFFFFFF;
        if (reg_name == "r10d") return regs.r10 & 0xFFFFFFFF;
        if (reg_name == "r11d") return regs.r11 & 0xFFFFFFFF;
        if (reg_name == "r12d") return regs.r12 & 0xFFFFFFFF;
        if (reg_name == "r13d") return regs.r13 & 0xFFFFFFFF;
        if (reg_name == "r14d") return regs.r14 & 0xFFFFFFFF;
        if (reg_name == "r15d") return regs.r15 & 0xFFFFFFFF;
        
        throw mx::Exception("Unknown 32-bit register name: " + reg_name);
    }

    uint16_t Process::get_register_16(const std::string &reg_name) const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "ax") return regs.rax & 0xFFFF;
        if (reg_name == "bx") return regs.rbx & 0xFFFF;
        if (reg_name == "cx") return regs.rcx & 0xFFFF;
        if (reg_name == "dx") return regs.rdx & 0xFFFF;
        if (reg_name == "si") return regs.rsi & 0xFFFF;
        if (reg_name == "di") return regs.rdi & 0xFFFF;
        if (reg_name == "bp") return regs.rbp & 0xFFFF;
        if (reg_name == "sp") return regs.rsp & 0xFFFF;
        if (reg_name == "r8w") return regs.r8 & 0xFFFF;
        if (reg_name == "r9w") return regs.r9 & 0xFFFF;
        if (reg_name == "r10w") return regs.r10 & 0xFFFF;
        if (reg_name == "r11w") return regs.r11 & 0xFFFF;
        if (reg_name == "r12w") return regs.r12 & 0xFFFF;
        if (reg_name == "r13w") return regs.r13 & 0xFFFF;
        if (reg_name == "r14w") return regs.r14 & 0xFFFF;
        if (reg_name == "r15w") return regs.r15 & 0xFFFF;
        
        throw mx::Exception("Unknown 16-bit register name: " + reg_name);
    }

    uint8_t Process::get_register_8(const std::string &reg_name) const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
    
        if (reg_name == "al") return regs.rax & 0xFF;
        if (reg_name == "ah") return (regs.rax >> 8) & 0xFF;
        if (reg_name == "bl") return regs.rbx & 0xFF;
        if (reg_name == "bh") return (regs.rbx >> 8) & 0xFF;
        if (reg_name == "cl") return regs.rcx & 0xFF;
        if (reg_name == "ch") return (regs.rcx >> 8) & 0xFF;
        if (reg_name == "dl") return regs.rdx & 0xFF;
        if (reg_name == "dh") return (regs.rdx >> 8) & 0xFF;
        if (reg_name == "sil") return regs.rsi & 0xFF;
        if (reg_name == "dil") return regs.rdi & 0xFF;
        if (reg_name == "bpl") return regs.rbp & 0xFF;
        if (reg_name == "spl") return regs.rsp & 0xFF;
        if (reg_name == "r8b") return regs.r8 & 0xFF;
        if (reg_name == "r9b") return regs.r9 & 0xFF;
        if (reg_name == "r10b") return regs.r10 & 0xFF;
        if (reg_name == "r11b") return regs.r11 & 0xFF;
        if (reg_name == "r12b") return regs.r12 & 0xFF;
        if (reg_name == "r13b") return regs.r13 & 0xFF;
        if (reg_name == "r14b") return regs.r14 & 0xFF;
        if (reg_name == "r15b") return regs.r15 & 0xFF;
        
        throw mx::Exception("Unknown 8-bit register name: " + reg_name);
    }

    std::vector<uint8_t> Process::read_memory(uint64_t address, size_t size) const {
        std::vector<uint8_t> result;
        size_t count = 0;
        while (count < size) {
            errno = 0;
            long word = ptrace(PTRACE_PEEKDATA, m_pid, address + count, nullptr);
            if (errno != 0) {
                throw mx::Exception::error("Failed to read memory");
            }
            size_t bytes_to_copy = std::min(sizeof(long), size - count);
            uint8_t *p = reinterpret_cast<uint8_t*>(&word);
            result.insert(result.end(), p, p + bytes_to_copy);
            count += bytes_to_copy;
        }
        return result;
    }

    void Process::write_memory(uint64_t address, const std::vector<uint8_t> &data) {
        size_t count = 0;
        size_t size = data.size();
        while (count < size) {
            long word = 0;
            size_t bytes_to_copy = std::min(sizeof(long), size - count);
            std::memcpy(&word, data.data() + count, bytes_to_copy);
            if (ptrace(PTRACE_POKEDATA, m_pid, address + count, word) == -1) {
                throw mx::Exception::error("Failed to write memory");
            }
            count += bytes_to_copy;
        }
    }

    uint64_t Process::get_pc() const {
        return get_register("rip");
    }

    uint8_t Process::get_original_instruction(uint64_t address) const {
        auto it = breakpoints.find(address);
        return (it != breakpoints.end()) ? it->second : 0;
    }

    void Process::handle_breakpoint_continue(uint64_t address) {
        uint8_t original_byte = breakpoints[address];
        long data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        long restored = (data & ~0xFF) | original_byte;
        ptrace(PTRACE_POKEDATA, m_pid, address, restored);
        ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
        int status;
        waitpid(m_pid, &status, 0);
        data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        ptrace(PTRACE_POKEDATA, m_pid, address, data_with_int3);
    }

    std::string Process::disassemble_instruction(uint64_t address) const {
        return "disassembly not implemented";
    }

    std::string Process::get_current_instruction() const {
        return "current instruction not implemented";
    }

    void Process::set_breakpoint(uint64_t address) {
        long data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        if (data == -1 && errno != 0)
            throw mx::Exception::error("Failed to read memory for breakpoint");

        breakpoints[address] = static_cast<uint8_t>(data & 0xFF);
        
        auto it = std::find(breakpoint_index.begin(), breakpoint_index.end(), address);
        if (it == breakpoint_index.end()) {
            breakpoint_index.push_back(address);
        }
        
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        if (ptrace(PTRACE_POKEDATA, m_pid, address, data_with_int3) == -1)
            throw mx::Exception::error("Failed to write breakpoint");
    }

    bool Process::remove_breakpoint(uint64_t address) {
        auto it = breakpoints.find(address);
        if (it == breakpoints.end()) return false;
        long data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        long restored = (data & ~0xFF) | it->second;
        ptrace(PTRACE_POKEDATA, m_pid, address, restored);        
        breakpoints.erase(it);
        auto index_it = std::find(breakpoint_index.begin(), breakpoint_index.end(), address);
        if (index_it != breakpoint_index.end()) {
            breakpoint_index.erase(index_it);
        }
        return true;
    }

    bool Process::remove_breakpoint_by_index(size_t index) {
        if (index == 0 || index > breakpoint_index.size()) {
            return false;
        }
        uint64_t address = breakpoint_index[index - 1];
        return remove_breakpoint(address);
    }

    std::vector<std::pair<size_t, uint64_t>> Process::get_breakpoints_with_index() const {
        std::vector<std::pair<size_t, uint64_t>> indexed_breakpoints;
        for (size_t i = 0; i < breakpoint_index.size(); ++i) {
            indexed_breakpoints.push_back(std::make_pair(i + 1, breakpoint_index[i])); 
        }
        return indexed_breakpoints;
    }

    uint64_t Process::get_breakpoint_address_by_index(size_t index) const {
        if (index == 0 || index > breakpoint_index.size()) {
            return 0;
        }
        return breakpoint_index[index - 1]; 
    }

    size_t Process::get_breakpoint_index_by_address(uint64_t address) const {
        auto it = std::find(breakpoint_index.begin(), breakpoint_index.end(), address);
        if (it != breakpoint_index.end()) {
            return std::distance(breakpoint_index.begin(), it) + 1; 
        }
        return 0; 
    }

    std::vector<std::pair<uint64_t, uint64_t>> Process::get_breakpoints() const {
        std::vector<std::pair<uint64_t, uint64_t>> addresses;
        for (const auto& bp : breakpoints) {
            addresses.push_back(std::make_pair(bp.first, bp.second));
        }
        return addresses;
    }

    
    bool Process::has_breakpoint(uint64_t address) const {
        return breakpoints.find(address) != breakpoints.end();
    }

    bool Process::is_running() const {
        if (m_pid <= 0) return false;
        int result = kill(m_pid, 0);
        return result == 0;
    }

    void Process::detach() {
        if (kill(m_pid, 0) == -1) {
            if (errno == ESRCH) {
                return;
            }
        }        
        if (ptrace(PTRACE_DETACH, m_pid, nullptr, 0) == -1) {
            throw mx::Exception::error("Failed to detach from process");
        }
    }

    void Process::set_register(const std::string &reg_name, uint64_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "rax") regs.rax = value;
        else if (reg_name == "rbx") regs.rbx = value;
        else if (reg_name == "rcx") regs.rcx = value;
        else if (reg_name == "rdx") regs.rdx = value;
        else if (reg_name == "rsi") regs.rsi = value;
        else if (reg_name == "rdi") regs.rdi = value;
        else if (reg_name == "rbp") regs.rbp = value;
        else if (reg_name == "rsp") regs.rsp = value;
        else if (reg_name == "rip") regs.rip = value;
        else if (reg_name == "r8") regs.r8 = value;
        else if (reg_name == "r9") regs.r9 = value;
        else if (reg_name == "r10") regs.r10 = value;
        else if (reg_name == "r11") regs.r11 = value;
        else if (reg_name == "r12") regs.r12 = value;
        else if (reg_name == "r13") regs.r13 = value;
        else if (reg_name == "r14") regs.r14 = value;
        else if (reg_name == "r15") regs.r15 = value;
        else {
            throw mx::Exception("Unknown register name: " + reg_name);
        }
        
        if (ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_32(const std::string &reg_name, uint32_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "eax") regs.rax = (regs.rax & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "ebx") regs.rbx = (regs.rbx & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "ecx") regs.rcx = (regs.rcx & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "edx") regs.rdx = (regs.rdx & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "esi") regs.rsi = (regs.rsi & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "edi") regs.rdi = (regs.rdi & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "ebp") regs.rbp = (regs.rbp & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "esp") regs.rsp = (regs.rsp & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r8d") regs.r8 = (regs.r8 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r9d") regs.r9 = (regs.r9 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r10d") regs.r10 = (regs.r10 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r11d") regs.r11 = (regs.r11 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r12d") regs.r12 = (regs.r12 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r13d") regs.r13 = (regs.r13 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r14d") regs.r14 = (regs.r14 & 0xFFFFFFFF00000000ULL) | value;
        else if (reg_name == "r15d") regs.r15 = (regs.r15 & 0xFFFFFFFF00000000ULL) | value;
        else {
            throw mx::Exception("Unknown 32-bit register name: " + reg_name);
        }
        
        if (ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_16(const std::string &reg_name, uint16_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        if (reg_name == "ax") regs.rax = (regs.rax & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "bx") regs.rbx = (regs.rbx & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "cx") regs.rcx = (regs.rcx & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "dx") regs.rdx = (regs.rdx & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "si") regs.rsi = (regs.rsi & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "di") regs.rdi = (regs.rdi & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "bp") regs.rbp = (regs.rbp & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "sp") regs.rsp = (regs.rsp & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r8w") regs.r8 = (regs.r8 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r9w") regs.r9 = (regs.r9 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r10w") regs.r10 = (regs.r10 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r11w") regs.r11 = (regs.r11 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r12w") regs.r12 = (regs.r12 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r13w") regs.r13 = (regs.r13 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r14w") regs.r14 = (regs.r14 & 0xFFFFFFFFFFFF0000ULL) | value;
        else if (reg_name == "r15w") regs.r15 = (regs.r15 & 0xFFFFFFFFFFFF0000ULL) | value;
        else {
            throw mx::Exception("Unknown 16-bit register name: " + reg_name);
        }
        
        if (ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_8(const std::string &reg_name, uint8_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        std::cout << "HERE\n";

        if (reg_name == "al") regs.rax = (regs.rax & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "ah") regs.rax = (regs.rax & 0xFFFFFFFFFFFF00FFULL) | (static_cast<uint64_t>(value) << 8);
        else if (reg_name == "bl") regs.rbx = (regs.rbx & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "bh") regs.rbx = (regs.rbx & 0xFFFFFFFFFFFF00FFULL) | (static_cast<uint64_t>(value) << 8);
        else if (reg_name == "cl") regs.rcx = (regs.rcx & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "ch") regs.rcx = (regs.rcx & 0xFFFFFFFFFFFF00FFULL) | (static_cast<uint64_t>(value) << 8);
        else if (reg_name == "dl") regs.rdx = (regs.rdx & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "dh") regs.rdx = (regs.rdx & 0xFFFFFFFFFFFF00FFULL) | (static_cast<uint64_t>(value) << 8);
        else if (reg_name == "sil") regs.rsi = (regs.rsi & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "dil") regs.rdi = (regs.rdi & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "bpl") regs.rbp = (regs.rbp & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "spl") regs.rsp = (regs.rsp & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r8b") regs.r8 = (regs.r8 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r9b") regs.r9 = (regs.r9 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r10b") regs.r10 = (regs.r10 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r11b") regs.r11 = (regs.r11 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r12b") regs.r12 = (regs.r12 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r13b") regs.r13 = (regs.r13 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r14b") regs.r14 = (regs.r14 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else if (reg_name == "r15b") regs.r15 = (regs.r15 & 0xFFFFFFFFFFFFFF00ULL) | value;
        else {
            throw mx::Exception("Unknown 8-bit register name: " + reg_name);
        }
        
        if (ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::print_all_registers() const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        
        std::cout << std::left << std::setfill(' ');
        
        std::cout << std::setw(8) << "RAX:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rax << std::dec
                  << "  EAX: 0x" << std::hex << std::setw(8) << (regs.rax & 0xFFFFFFFF) << std::dec
                  << "  AX: 0x" << std::hex << std::setw(4) << (regs.rax & 0xFFFF) << std::dec
                  << "  AL: 0x" << std::hex << std::setw(2) << (regs.rax & 0xFF) << std::dec
                  << "  AH: 0x" << std::hex << std::setw(2) << ((regs.rax >> 8) & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RBX:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rbx << std::dec
                  << "  EBX: 0x" << std::hex << std::setw(8) << (regs.rbx & 0xFFFFFFFF) << std::dec
                  << "  BX: 0x" << std::hex << std::setw(4) << (regs.rbx & 0xFFFF) << std::dec
                  << "  BL: 0x" << std::hex << std::setw(2) << (regs.rbx & 0xFF) << std::dec
                  << "  BH: 0x" << std::hex << std::setw(2) << ((regs.rbx >> 8) & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RCX:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rcx << std::dec
                  << "  ECX: 0x" << std::hex << std::setw(8) << (regs.rcx & 0xFFFFFFFF) << std::dec
                  << "  CX: 0x" << std::hex << std::setw(4) << (regs.rcx & 0xFFFF) << std::dec
                  << "  CL: 0x" << std::hex << std::setw(2) << (regs.rcx & 0xFF) << std::dec
                  << "  CH: 0x" << std::hex << std::setw(2) << ((regs.rcx >> 8) & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RDX:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rdx << std::dec
                  << "  EDX: 0x" << std::hex << std::setw(8) << (regs.rdx & 0xFFFFFFFF) << std::dec
                  << "  DX: 0x" << std::hex << std::setw(4) << (regs.rdx & 0xFFFF) << std::dec
                  << "  DL: 0x" << std::hex << std::setw(2) << (regs.rdx & 0xFF) << std::dec
                  << "  DH: 0x" << std::hex << std::setw(2) << ((regs.rdx >> 8) & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RSI:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rsi << std::dec
                  << "  ESI: 0x" << std::hex << std::setw(8) << (regs.rsi & 0xFFFFFFFF) << std::dec
                  << "  SI: 0x" << std::hex << std::setw(4) << (regs.rsi & 0xFFFF) << std::dec
                  << "  SIL: 0x" << std::hex << std::setw(2) << (regs.rsi & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RDI:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rdi << std::dec
                  << "  EDI: 0x" << std::hex << std::setw(8) << (regs.rdi & 0xFFFFFFFF) << std::dec
                  << "  DI: 0x" << std::hex << std::setw(4) << (regs.rdi & 0xFFFF) << std::dec
                  << "  DIL: 0x" << std::hex << std::setw(2) << (regs.rdi & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RBP:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rbp << std::dec
                  << "  EBP: 0x" << std::hex << std::setw(8) << (regs.rbp & 0xFFFFFFFF) << std::dec
                  << "  BP: 0x" << std::hex << std::setw(4) << (regs.rbp & 0xFFFF) << std::dec
                  << "  BPL: 0x" << std::hex << std::setw(2) << (regs.rbp & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RSP:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rsp << std::dec
                  << "  ESP: 0x" << std::hex << std::setw(8) << (regs.rsp & 0xFFFFFFFF) << std::dec
                  << "  SP: 0x" << std::hex << std::setw(4) << (regs.rsp & 0xFFFF) << std::dec
                  << "  SPL: 0x" << std::hex << std::setw(2) << (regs.rsp & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "RIP:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.rip << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R8:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r8 << std::dec
                  << "  R8D: 0x" << std::hex << std::setw(8) << (regs.r8 & 0xFFFFFFFF) << std::dec
                  << "  R8W: 0x" << std::hex << std::setw(4) << (regs.r8 & 0xFFFF) << std::dec
                  << "  R8B: 0x" << std::hex << std::setw(2) << (regs.r8 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R9:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r9 << std::dec
                  << "  R9D: 0x" << std::hex << std::setw(8) << (regs.r9 & 0xFFFFFFFF) << std::dec
                  << "  R9W: 0x" << std::hex << std::setw(4) << (regs.r9 & 0xFFFF) << std::dec
                  << "  R9B: 0x" << std::hex << std::setw(2) << (regs.r9 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R10:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r10 << std::dec
                  << "  R10D: 0x" << std::hex << std::setw(8) << (regs.r10 & 0xFFFFFFFF) << std::dec
                  << "  R10W: 0x" << std::hex << std::setw(4) << (regs.r10 & 0xFFFF) << std::dec
                  << "  R10B: 0x" << std::hex << std::setw(2) << (regs.r10 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R11:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r11 << std::dec
                  << "  R11D: 0x" << std::hex << std::setw(8) << (regs.r11 & 0xFFFFFFFF) << std::dec
                  << "  R11W: 0x" << std::hex << std::setw(4) << (regs.r11 & 0xFFFF) << std::dec
                  << "  R11B: 0x" << std::hex << std::setw(2) << (regs.r11 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R12:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r12 << std::dec
                  << "  R12D: 0x" << std::hex << std::setw(8) << (regs.r12 & 0xFFFFFFFF) << std::dec
                  << "  R12W: 0x" << std::hex << std::setw(4) << (regs.r12 & 0xFFFF) << std::dec
                  << "  R12B: 0x" << std::hex << std::setw(2) << (regs.r12 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R13:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r13 << std::dec
                  << "  R13D: 0x" << std::hex << std::setw(8) << (regs.r13 & 0xFFFFFFFF) << std::dec
                  << "  R13W: 0x" << std::hex << std::setw(4) << (regs.r13 & 0xFFFF) << std::dec
                  << "  R13B: 0x" << std::hex << std::setw(2) << (regs.r13 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R14:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r14 << std::dec
                  << "  R14D: 0x" << std::hex << std::setw(8) << (regs.r14 & 0xFFFFFFFF) << std::dec
                  << "  R14W: 0x" << std::hex << std::setw(4) << (regs.r14 & 0xFFFF) << std::dec
                  << "  R14B: 0x" << std::hex << std::setw(2) << (regs.r14 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "R15:" << "0x" << std::hex << std::setfill('0') << std::setw(16) << regs.r15 << std::dec
                  << "  R15D: 0x" << std::hex << std::setw(8) << (regs.r15 & 0xFFFFFFFF) << std::dec
                  << "  R15W: 0x" << std::hex << std::setw(4) << (regs.r15 & 0xFFFF) << std::dec
                  << "  R15B: 0x" << std::hex << std::setw(2) << (regs.r15 & 0xFF) << std::dec << std::endl;
        
        std::cout << std::setw(8) << "EFLAGS:" << "0x" << std::hex << std::setfill('0') << std::setw(8) << regs.eflags << std::dec << std::endl;
    }
}