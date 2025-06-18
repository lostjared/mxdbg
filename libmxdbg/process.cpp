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
        if (waitpid(pid, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for process after attaching");
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
            std::cout << "Process killed by signal: " << WTERMSIG(status) << std::endl;
            return;
        } else if (WIFSTOPPED(status)) {
            int signal = WSTOPSIG(status);
            std::cout << "DEBUG: Process stopped by signal: " << signal << std::endl;
            
            if (signal == SIGTRAP) {
                uint64_t pc = get_pc();           
                if (breakpoints.find(pc) != breakpoints.end()) {
                    std::cout << "Breakpoint hit at 0x" << std::hex << pc << std::dec << std::endl;
                    return;
                } else if (breakpoints.find(pc - 1) != breakpoints.end()) {
                    std::cout << "Breakpoint hit at 0x" << std::hex << (pc - 1) << std::dec << std::endl;
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
        
        ptrace(PTRACE_CONT, m_pid, nullptr, 0);
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

    uint64_t Process::get_register(const std::string &reg_name_) const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
        std::string reg_name = reg_name_;
        std::transform(reg_name.begin(), reg_name.end(), reg_name.begin(), ::tolower);
        if (reg_name == "rflags") return regs.eflags;
        if (reg_name == "rip") return regs.rip;
        if (reg_name == "rsp") return regs.rsp;
        if (reg_name == "rbp") return regs.rbp;
        if (reg_name == "rax") return regs.rax;
        if (reg_name == "rbx") return regs.rbx;
        if (reg_name == "rcx") return regs.rcx;
        if (reg_name == "rdx") return regs.rdx;
        if (reg_name == "rsi") return regs.rsi;
        if (reg_name == "rdi") return regs.rdi;
        throw mx::Exception("Unknown register: " + reg_name);
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
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        if (ptrace(PTRACE_POKEDATA, m_pid, address, data_with_int3) == -1)
            throw mx::Exception::error("Failed to write breakpoint");
    }

    void Process::remove_breakpoint(uint64_t address) {
        auto it = breakpoints.find(address);
        if (it == breakpoints.end()) return;
        long data = ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        long restored = (data & ~0xFF) | it->second;
        ptrace(PTRACE_POKEDATA, m_pid, address, restored);
        breakpoints.erase(it);
    }

    bool Process::has_breakpoint(uint64_t address) const {
        return breakpoints.find(address) != breakpoints.end();
    }

    std::vector<uint64_t> Process::get_breakpoints() const {
        std::vector<uint64_t> addresses;
        for (const auto& bp : breakpoints) {
            addresses.push_back(bp.first);
        }
        return addresses;
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
}