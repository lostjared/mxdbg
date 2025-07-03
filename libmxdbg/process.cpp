/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/

#include"mxdbg/process.hpp"
#include"mxdbg/pipe.hpp"
#include"mxdbg/exception.hpp"
#include"mxdbg/expr.hpp"
#include<sys/ptrace.h>
#include <sys/wait.h>
#ifndef __WALL
#define __WALL 0x40000000
#endif
#include<sys/user.h>
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
#include<random>

namespace mx {
    
    
    Process::Process(Process&& proc) : m_pid(proc.m_pid), current_thread_id(proc.current_thread_id), index_(proc.index_) {
        proc.m_pid = -1;
        proc.current_thread_id = -1;
        proc.index_ = 0;
    }
   
    Process &Process::operator=(Process&& other) {
        if (this != &other) {
            m_pid = other.m_pid;
            current_thread_id = other.current_thread_id;
            index_ = other.index_;
            other.m_pid = -1; 
            other.current_thread_id = -1;
            other.index_ = 0;
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
            if (ptrace(PTRACE_SETOPTIONS, pid, nullptr, 
                      PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEFORK) == -1) {
                std::cerr << "Warning: Could not enable thread tracing" << std::endl;
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
         if (ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACECLONE) == -1) {
            std::cerr << "Warning: Could not enable thread tracing" << std::endl;
        }  
        return std::unique_ptr<Process>(new Process(pid));
    }

    void Process::continue_execution() {
        auto thread_ids = get_thread_ids();
        for (pid_t tid : thread_ids) {
            if (ptrace(PTRACE_CONT, tid, nullptr, 0) == -1) {
                if (errno != ESRCH) { 
                    throw mx::Exception::error("Failed to continue thread " + std::to_string(tid));
                }
            }
        }
    }

     void Process::break_if(uint64_t address, const std::string &condition) {
        if (!is_running()) {
            std::cout << "No process attached or running." << std::endl;
            return;
        }
        
        if (condition.empty()) {
            std::cout << "Usage: break_if <address> <condition>" << std::endl;
            std::cout << "Example: break_if 0x401234 rax == 5" << std::endl;
            return;
        }
        
        try {
            long data = ptrace(PTRACE_PEEKDATA, get_current_thread(), address, nullptr);
            if (data == -1 && errno != 0) {
                throw mx::Exception::error("Failed to read memory for breakpoint");
            }
            
            uint8_t original_byte = static_cast<uint8_t>(data & 0xFF);
            
            if (conditional_breakpoints.find(address) != conditional_breakpoints.end()) {
                std::cout << "Breakpoint already exists at " << format_hex64(address) << std::endl;
                std::cout << "Updating condition..." << std::endl;
                conditional_breakpoints[address].condition = condition;
                conditional_breakpoints[address].is_conditional = true;
                return;
            }
            
            long data_with_int3 = (data & ~0xFF) | 0xCC;
            if (ptrace(PTRACE_POKEDATA, get_current_thread(), address, data_with_int3) == -1) {
                throw mx::Exception::error("Failed to write breakpoint");
            }
            conditional_breakpoints[address] = ConditionalBreakpoint(address, original_byte, condition);
            std::cout << "Conditional breakpoint set at: " << format_hex64(address) << std::endl;
            std::cout << "Condition: " << condition << std::endl; 
        } catch (const std::exception& e) {
            std::cerr << "Error setting conditional breakpoint: " << e.what() << std::endl;
        }
    }

    uint64_t Process::expression(const std::string &text) {
        try {

            if (is_running()) {
                try {
                    using namespace expr_parser;
                    struct user_regs_struct regs;
                    pid_t current_tid = get_current_thread();
                    if (ptrace(PTRACE_GETREGS, current_tid, nullptr, &regs) == 0) {
                        vars["rax"] = regs.rax;
                        vars["rbx"] = regs.rbx;
                        vars["rcx"] = regs.rcx;
                        vars["rdx"] = regs.rdx;
                        vars["rsi"] = regs.rsi;
                        vars["rdi"] = regs.rdi;
                        vars["rbp"] = regs.rbp;
                        vars["rsp"] = regs.rsp;
                        vars["r8"] = regs.r8;
                        vars["r9"] = regs.r9;
                        vars["r10"] = regs.r10;
                        vars["r11"] = regs.r11;
                        vars["r12"] = regs.r12;
                        vars["r13"] = regs.r13;
                        vars["r14"] = regs.r14;
                        vars["r15"] = regs.r15;
                        vars["rip"] = regs.rip;
                        vars["eflags"] = regs.eflags;
                        vars["pc"] = regs.rip;
                        vars["ip"] = regs.rip;
                        vars["sp"] = regs.rsp;
                        vars["bp"] = regs.rbp;
                        vars["flags"] = regs.eflags;
                        vars["cs"] = regs.cs;
                        vars["ss"] = regs.ss;
                        vars["ds"] = regs.ds;
                        vars["es"] = regs.es;
                        vars["fs"] = regs.fs;
                        vars["gs"] = regs.gs;
                        vars["orig_rax"] = regs.orig_rax;
                        vars["fs_base"] = regs.fs_base;
                        vars["gs_base"] = regs.gs_base;
                        vars["eax"] = regs.rax & 0xFFFFFFFF;
                        vars["ebx"] = regs.rbx & 0xFFFFFFFF;
                        vars["ecx"] = regs.rcx & 0xFFFFFFFF;
                        vars["edx"] = regs.rdx & 0xFFFFFFFF;
                        vars["esi"] = regs.rsi & 0xFFFFFFFF;
                        vars["edi"] = regs.rdi & 0xFFFFFFFF;
                        vars["ebp"] = regs.rbp & 0xFFFFFFFF;
                        vars["esp"] = regs.rsp & 0xFFFFFFFF;
                        vars["r8d"] = regs.r8 & 0xFFFFFFFF;
                        vars["r9d"] = regs.r9 & 0xFFFFFFFF;
                        vars["r10d"] = regs.r10 & 0xFFFFFFFF;
                        vars["r11d"] = regs.r11 & 0xFFFFFFFF;
                        vars["r12d"] = regs.r12 & 0xFFFFFFFF;
                        vars["r13d"] = regs.r13 & 0xFFFFFFFF;
                        vars["r14d"] = regs.r14 & 0xFFFFFFFF;
                        vars["r15d"] = regs.r15 & 0xFFFFFFFF;
                        vars["ax"] = regs.rax & 0xFFFF;
                        vars["bx"] = regs.rbx & 0xFFFF;
                        vars["cx"] = regs.rcx & 0xFFFF;
                        vars["dx"] = regs.rdx & 0xFFFF;
                        vars["si"] = regs.rsi & 0xFFFF;
                        vars["di"] = regs.rdi & 0xFFFF;
                        vars["r8w"] = regs.r8 & 0xFFFF;
                        vars["r9w"] = regs.r9 & 0xFFFF;
                        vars["r10w"] = regs.r10 & 0xFFFF;
                        vars["r11w"] = regs.r11 & 0xFFFF;
                        vars["r12w"] = regs.r12 & 0xFFFF;
                        vars["r13w"] = regs.r13 & 0xFFFF;
                        vars["r14w"] = regs.r14 & 0xFFFF;
                        vars["r15w"] = regs.r15 & 0xFFFF;
                        vars["al"] = regs.rax & 0xFF;
                        vars["ah"] = (regs.rax >> 8) & 0xFF;
                        vars["bl"] = regs.rbx & 0xFF;
                        vars["bh"] = (regs.rbx >> 8) & 0xFF;
                        vars["cl"] = regs.rcx & 0xFF;
                        vars["ch"] = (regs.rcx >> 8) & 0xFF;
                        vars["dl"] = regs.rdx & 0xFF;
                        vars["dh"] = (regs.rdx >> 8) & 0xFF;
                        vars["sil"] = regs.rsi & 0xFF;
                        vars["dil"] = regs.rdi & 0xFF;
                        vars["bpl"] = regs.rbp & 0xFF;
                        vars["spl"] = regs.rsp & 0xFF;
                        vars["r8b"] = regs.r8 & 0xFF;
                        vars["r9b"] = regs.r9 & 0xFF;
                        vars["r10b"] = regs.r10 & 0xFF;
                        vars["r11b"] = regs.r11 & 0xFF;
                        vars["r12b"] = regs.r12 & 0xFF;
                        vars["r13b"] = regs.r13 & 0xFF;
                        vars["r14b"] = regs.r14 & 0xFF;
                        vars["r15b"] = regs.r15 & 0xFF;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Could not read registers for expression: " << e.what() << std::endl;
                }
            }
            expr_parser::ExprLexer lexer(text);
            expr_parser::ExprParser parser(lexer);
            uint64_t result = parser.parse();
            std::cout << "The result: ";
            if(color_)
                std::cout << Color::BOLD;
            std::cout << format_hex64(result) << " | " << std::dec << result << "\n";
            if(color_)
                std::cout << Color::RESET;
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Error on expression: " << e.what() << "\n";
            return 0;
        }
        throw mx::Exception("Processing Expression failed...\n");
    }


    void Process::wait_for_stop() {
        int status;
        pid_t waited_pid;
        
        while (true) { 
            if ((waited_pid = waitpid(-1, &status, __WALL)) == -1) {
                throw mx::Exception::error("Failed to wait for process");
            }
            
            if (WIFEXITED(status)) {
                std::cout << "Process/Thread " << waited_pid << " exited with code: " 
                        << WEXITSTATUS(status) << std::endl;
                
                if (waited_pid == m_pid) {
                    exited_ = true;
                    return; 
                }
                
                continue;
            }
            
            if (WIFSIGNALED(status)) {
                std::cout << "Process/Thread " << waited_pid << " killed by signal: " 
                        << format_signal(WTERMSIG(status)) << std::endl;
                
                if (waited_pid == m_pid) {
                    exited_ = true;
                    return; 
                }
                
                continue;
            }
        
            if (WIFSTOPPED(status)) {
                int signal = WSTOPSIG(status);
                
                if (signal == SIGTRAP) {
                    int ptrace_event = (status >> 16) & 0xffff;
                    
                    if (ptrace_event == PTRACE_EVENT_CLONE) {
                        unsigned long new_thread_id;
                        if (ptrace(PTRACE_GETEVENTMSG, waited_pid, nullptr, &new_thread_id) == 0) {
                            std::cout << "New thread created: " << new_thread_id 
                                    << " (parent: " << waited_pid << ")" << std::endl;
                        } else {
                            std::cout << "New thread created (couldn't get TID)" << std::endl;
                        }
                        
                        ptrace(PTRACE_CONT, waited_pid, nullptr, 0);
                        continue; 
                    }

                    else if (ptrace_event == PTRACE_EVENT_EXEC) {
                        std::cout << "Process " << waited_pid << " executed new program" << std::endl;
                        ptrace(PTRACE_CONT, waited_pid, nullptr, 0);
                        continue; 
                    }
                    else if (ptrace_event == PTRACE_EVENT_FORK) {
                        unsigned long new_pid;
                        if (ptrace(PTRACE_GETEVENTMSG, waited_pid, nullptr, &new_pid) == 0) {
                            std::cout << "New process forked: " << new_pid 
                                    << " (parent: " << waited_pid << ")" << std::endl;
                        }
                        ptrace(PTRACE_CONT, waited_pid, nullptr, 0);
                        continue; 
                    }
                    
                    if (waited_pid != current_thread_id) {
                        struct user_regs_struct test_regs;
                        if (ptrace(PTRACE_GETREGS, waited_pid, nullptr, &test_regs) == 0) {
                            std::cout << "Thread " << waited_pid << " stopped, switching context" << std::endl;
                            current_thread_id = waited_pid;
                        } else {
                            std::cout << "Thread " << waited_pid << " event, but can't access registers - continuing" << std::endl;
                            ptrace(PTRACE_CONT, waited_pid, nullptr, 0);
                            continue;
                        }
                    }
        
                    uint64_t pc = get_pc();

                    if(skip_next_breakpoint == pc) {
                        skip_next_breakpoint = 0;
                        ptrace(PTRACE_CONT, current_thread_id, nullptr, 0);
                        continue;
                    }

                    auto bp_it = conditional_breakpoints.find(pc);
                    if (bp_it == conditional_breakpoints.end()) {
                        bp_it = conditional_breakpoints.find(pc - 1);  
                        if (bp_it != conditional_breakpoints.end()) {
                            set_pc(pc - 1);  
                            pc = pc - 1;
                        }
                    }

                    if (bp_it != conditional_breakpoints.end()) {
                            const ConditionalBreakpoint& bp = bp_it->second;

                        if (bp.is_conditional) {
                            bool condition_met = false;
                            try {
                                std::ostringstream old_cout;
                                std::streambuf* cout_buf = std::cout.rdbuf(); 
                                std::cout.rdbuf(old_cout.rdbuf());
                                try {
                                    uint64_t result = expression(bp.condition);
                                    condition_met = (result != 0);  
                                }
                                catch (const std::exception& e) {
                                    condition_met = false;
                                }
                                std::cout.rdbuf(cout_buf); 
                                
                            } catch (const std::exception& e) {
                                std::cerr << "Error processing breakpoint condition '" << bp.condition 
                                        << "': " << e.what() << std::endl;
                                condition_met = false;  
                            }
                            
                            if (!condition_met) {
                                std::cout << "Conditional breakpoint at " << format_hex64(pc) 
                                        << " hit, but condition '" << bp.condition << "' is false or invalid. Continuing..." << std::endl;
                                
                                handle_conditional_breakpoint_continue(pc);
                                continue;  
                            } else {
                                
                                handle_conditional_breakpoint_continue(pc);
                                
                                std::cout << "=== CONDITIONAL BREAKPOINT HIT ===" << std::endl;
                                std::cout << "Thread: " << current_thread_id << std::endl;
                                std::cout << "Address: " << format_hex64(pc) << std::endl;
                                std::cout << "Condition: " << bp.condition << " (TRUE)" << std::endl;
                                return;  
                            }
                        }
                    }

                    if (breakpoints.find(pc) != breakpoints.end()) {
                        std::cout << "=== BREAKPOINT HIT ===" << std::endl;
                        std::cout << "Thread: " << current_thread_id << std::endl;
                        std::cout << "Address: " << format_hex64(pc) << std::endl;
                        return; 
                    } 
                    
                    else if (breakpoints.find(pc - 1) != breakpoints.end()) {
                        std::cout << "=== BREAKPOINT HIT ===" << std::endl;
                        std::cout << "Thread: " << current_thread_id << std::endl;
                        std::cout << "Address: " << format_hex64(pc - 1) << std::endl;
                        set_pc(pc - 1);
                        return; 
                    }
                    
                    
                    unsigned long dr6 = 0;
                    if ((dr6 = ptrace(PTRACE_PEEKUSER, current_thread_id, 
                                    offsetof(user, u_debugreg[6]), nullptr)) != -1) {
                        if (dr6 & 0xF) { 
                            std::cout << "=== WATCHPOINT HIT ===" << std::endl;
                            std::cout << "Address: " << format_hex64(get_pc()) << std::endl;
                            ptrace(PTRACE_POKEUSER, current_thread_id, 
                                offsetof(user, u_debugreg[6]), 0);
                            
                            return; 
                        }
                    }
                    
                    std::cout << "SIGTRAP received (not breakpoint/watchpoint)" << std::endl;
                    return; 
                }
                
                
                else if (signal == SIGINT || signal == SIGTERM || signal == SIGSEGV || 
                        signal == SIGFPE || signal == SIGILL || signal == SIGABRT) {
                    std::cout << "=== PROGRAM STOPPED BY SIGNAL ===" << std::endl;
                    std::cout << "Thread: " << current_thread_id << std::endl;
                    std::cout << "Signal: " << format_signal(signal) << std::endl;
                    return; 
                }
                
                
                else {
                    std::cout << "Thread " << waited_pid << " received signal " 
                            << format_signal(signal) << ", continuing" << std::endl;
                    ptrace(PTRACE_CONT, waited_pid, nullptr, signal);
                    continue; 
                }
            }
            
            
            std::cout << "Unexpected wait status: " << status << std::endl;
            return; 
        }
    }
    
    void Process::mark_as_exited() {
        exited_ = true;
    }

    void Process::handle_conditional_breakpoint_continue(uint64_t address) {
        skip_next_breakpoint = address; 
        auto bp_it = conditional_breakpoints.find(address);
        if (bp_it == conditional_breakpoints.end()) {
            return;
        }
        const ConditionalBreakpoint& bp = bp_it->second;
        long data = ptrace(PTRACE_PEEKDATA, get_current_thread(), address, nullptr);
        long restored = (data & ~0xFF) | bp.original_byte;
        ptrace(PTRACE_POKEDATA, get_current_thread(), address, restored);
        ptrace(PTRACE_SINGLESTEP, get_current_thread(), nullptr, nullptr);
        int status;
        waitpid(get_current_thread(), &status, 0);
        data = ptrace(PTRACE_PEEKDATA, get_current_thread(), address, nullptr);
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        ptrace(PTRACE_POKEDATA, get_current_thread(), address, data_with_int3);
        for (pid_t tid : get_thread_ids()) {
            if (ptrace(PTRACE_CONT, tid, nullptr, 0) == -1 && errno != ESRCH) {
                std::cerr << "Warning: Failed to continue thread " << tid << std::endl;
            }
        }
    }

    std::string Process::disassemble_instruction(uint64_t address, const std::vector<uint8_t>& bytes) {
        try {
            std::string random_name;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 25);  
            for(size_t i = 0; i < 10; ++i) {
                random_name += 'a' + dis(gen);
            }
            std::string temp_file = "/tmp/mxdbg_watchpoint_" + random_name + ".bin";
            class FileRemoval {
            public:
                FileRemoval(const std::string &text) : fn(text) {}
                ~FileRemoval() {
                    if(std::filesystem::exists(fn)) {
                        std::filesystem::remove(fn);
                    }
                }
            private:
                std::string fn;
            };
            std::ofstream temp(temp_file, std::ios::binary);
            temp.write(reinterpret_cast<const char*>(bytes.data()), std::min(bytes.size(), (size_t)15));
            temp.close();
            FileRemoval rmv_file(temp_file);
            std::string cmd = "objdump -D -b binary -m i386:x86-64 " + temp_file + " 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            std::string result = "<unknown instruction>";
            if (pipe) {
                char buffer[512];
                std::string output;
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                pclose(pipe);
                std::istringstream ss(output);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.find("   0:") != std::string::npos) {
                        size_t colon_pos = line.find(":");
                        size_t tab_pos = line.find('\t', colon_pos);
                        
                        if (colon_pos != std::string::npos && tab_pos != std::string::npos) {
                            std::string hex_part = line.substr(colon_pos + 1, tab_pos - colon_pos - 1);
                            std::string instr_part = line.substr(tab_pos + 1);
                            if (!instr_part.empty() && instr_part.back() == '\n') {
                                instr_part.pop_back();
                            }               
                            result = hex_part + " -> " + instr_part;
                        }
                        break;
                    }
                }
            }
            if(std::filesystem::exists(temp_file)) {
                std::filesystem::remove(temp_file);
            }
            return result;       
        } catch (const std::exception& e) {
            return "<disassembly failed: " + std::string(e.what()) + ">";
        }
    }

    void Process::single_step() {
        uint64_t pc = get_pc();
        
        if (has_breakpoint(pc)) {
            handle_breakpoint_step(pc);
        } else {
            if (ptrace(PTRACE_SINGLESTEP, current_thread_id, nullptr, nullptr) == -1) {
                throw mx::Exception::error("Failed to single step process");
            }
            is_single_stepping = true;
        }
    }

    void Process::handle_breakpoint_step(uint64_t address) {
        uint8_t original_byte = breakpoints[address];
        long data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        long restored = (data & ~0xFF) | original_byte;
        ptrace(PTRACE_POKEDATA, current_thread_id, address, restored);
        
        ptrace(PTRACE_SINGLESTEP, current_thread_id, nullptr, nullptr);
        is_single_stepping = true;
        
    }
    
    void Process::handle_thread_breakpoint_continue(uint64_t address) {
        auto it = conditional_breakpoints.find(address);
        if (it == conditional_breakpoints.end()) {
            return;
        }
        
        const ConditionalBreakpoint& bp = it->second;
        long data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        long restored = (data & ~0xFF) | bp.original_byte;
        ptrace(PTRACE_POKEDATA, current_thread_id, address, restored);
        ptrace(PTRACE_SINGLESTEP, current_thread_id, nullptr, nullptr);
        int status;
        waitpid(current_thread_id, &status, 0);
        if (WIFEXITED(status)) {
            exited_ = true;
            std::cout << "Process exited with code: " << WEXITSTATUS(status) << std::endl;
            return;
        }
        uint64_t new_pc = get_pc();
        if (new_pc != address) {
            ptrace(PTRACE_CONT, current_thread_id, nullptr, nullptr);
        } else {
            data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
            long data_with_int3 = (data & ~0xFF) | 0xCC;
            ptrace(PTRACE_POKEDATA, current_thread_id, address, data_with_int3);
            ptrace(PTRACE_CONT, current_thread_id, nullptr, nullptr);
        }
    }

    void Process::wait_for_single_step() {
        int status;
        if (waitpid(current_thread_id, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for single step");
        }
        
        if (is_single_stepping) {
            uint64_t prev_pc = get_pc() - 1; 
            auto it = breakpoints.find(prev_pc);
            if (it != breakpoints.end()) {
                long data = ptrace(PTRACE_PEEKDATA, current_thread_id, prev_pc, nullptr);
                long data_with_int3 = (data & ~0xFF) | 0xCC;
                ptrace(PTRACE_POKEDATA, current_thread_id, prev_pc, data_with_int3);
            }
            is_single_stepping = false;
        }
    }

    void Process::set_pc(uint64_t address) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers");
        }
        regs.rip = address;
        if (ptrace(PTRACE_SETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
            long word = ptrace(PTRACE_PEEKDATA, current_thread_id, address + count, nullptr);
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
            if (ptrace(PTRACE_POKEDATA, current_thread_id, address + count, word) == -1) {
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
        long data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        long restored = (data & ~0xFF) | original_byte;
        ptrace(PTRACE_POKEDATA, current_thread_id, address, restored);
        ptrace(PTRACE_SINGLESTEP,current_thread_id, nullptr, nullptr);
        int status;
        waitpid(current_thread_id, &status, 0);
        data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        ptrace(PTRACE_POKEDATA, current_thread_id, address, data_with_int3);
    }

    std::string Process::disassemble_instruction(uint64_t address) const {
        return "disassembly not implemented";
    }

    std::string Process::get_current_instruction() const {
        return "current instruction not implemented";
    }

    void Process::set_breakpoint(uint64_t address) {
        long data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        if (data == -1 && errno != 0)
            throw mx::Exception::error("Failed to read memory for breakpoint");

        breakpoints[address] = static_cast<uint8_t>(data & 0xFF);
        
        auto it = std::find(breakpoint_index.begin(), breakpoint_index.end(), address);
        if (it == breakpoint_index.end()) {
            breakpoint_index.push_back(address);
        }
        
        long data_with_int3 = (data & ~0xFF) | 0xCC;
        if (ptrace(PTRACE_POKEDATA, current_thread_id, address, data_with_int3) == -1)
            throw mx::Exception::error("Failed to write breakpoint");
    }

    bool Process::remove_breakpoint(uint64_t address) {
        auto it = breakpoints.find(address);
        if (it == breakpoints.end()) return false;
        long data = ptrace(PTRACE_PEEKDATA, current_thread_id, address, nullptr);
        long restored = (data & ~0xFF) | it->second;
        ptrace(PTRACE_POKEDATA, current_thread_id, address, restored);        
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
        return breakpoints.find(address) != breakpoints.end() || 
            conditional_breakpoints.find(address) != conditional_breakpoints.end();
    }

    bool Process::has_conditional_breakpoint(uint64_t address) const {
        return conditional_breakpoints.find(address) != conditional_breakpoints.end();
    }

    bool Process::is_running() const {
        if (m_pid <= 0 || exited_) return false;
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
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        
        if (ptrace(PTRACE_SETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_32(const std::string &reg_name, uint32_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        
        if (ptrace(PTRACE_SETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_16(const std::string &reg_name, uint16_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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
        
        if (ptrace(PTRACE_SETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::set_register_8(const std::string &reg_name, uint8_t value) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to get registers for process");
        }
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
        
        if (ptrace(PTRACE_SETREGS, current_thread_id, nullptr, &regs) == -1) {
            throw mx::Exception::error("Failed to set registers");
        }
    }

    void Process::print_all_registers() const {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
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

    bool Process::set_watchpoint(uint64_t address, size_t size, WatchType type) {
        for (const auto& wp : watchpoints_) {
            if (wp.address == address) {
                return false; 
            }
        }
        
        try {
            if (watchpoints_.size() >= 4) {
                throw Exception("Maximum number of hardware watchpoints (4) exceeded");
            }
            
            user_regs_struct regs;
            if (ptrace(PTRACE_GETREGS, current_thread_id, nullptr, &regs) == -1) {
                throw Exception("Failed to get registers for watchpoint");
            }
            
            int dr_slot = watchpoints_.size(); 
            unsigned long dr_addr = address;
            
            unsigned long dr_offset;
            switch (dr_slot) {
                case 0: dr_offset = offsetof(user, u_debugreg[0]); break;
                case 1: dr_offset = offsetof(user, u_debugreg[1]); break;
                case 2: dr_offset = offsetof(user, u_debugreg[2]); break;
                case 3: dr_offset = offsetof(user, u_debugreg[3]); break;
                default: throw Exception("Invalid debug register slot");
            }
            
            if (ptrace(PTRACE_POKEUSER, current_thread_id, dr_offset, dr_addr) == -1) {
                throw Exception("Failed to set debug address register");
            }
            
            unsigned long dr7;
            if ((dr7 = ptrace(PTRACE_PEEKUSER, current_thread_id, 
                            offsetof(user, u_debugreg[7]), nullptr)) == -1) {
                throw Exception("Failed to read DR7 register");
            }
            
            unsigned long dr7_config = 0;
            dr7_config |= (3UL << (dr_slot * 2)); 
            unsigned long rw_bits = 0;
            switch (type) {
                case WatchType::WRITE: rw_bits = 1; break;  
                case WatchType::READ:  rw_bits = 3; break;  
                case WatchType::ACCESS: rw_bits = 3; break; 
            }
            dr7_config |= (rw_bits << (16 + dr_slot * 4));
            unsigned long len_bits = 0;
            switch (size) {
                case 1: len_bits = 0; break; 
                case 2: len_bits = 1; break; 
                case 4: len_bits = 3; break; 
                case 8: len_bits = 2; break; 
                default: throw Exception("Invalid watchpoint size");
            }
            dr7_config |= (len_bits << (18 + dr_slot * 4));
            
            dr7 |= dr7_config;
            
            if (ptrace(PTRACE_POKEUSER,current_thread_id, 
                    offsetof(user, u_debugreg[7]), dr7) == -1) {
                throw Exception("Failed to set DR7 register");
            }
    
            Watchpoint wp;
            wp.address = address;
            wp.size = size;
            wp.type = type;
            wp.description = "Watchpoint at " + format_hex64(address);
            watchpoints_.push_back(wp);
            return true;
            
        } catch (const std::exception& e) {
            throw Exception("Failed to set watchpoint: " + std::string(e.what()));
        }
    }

    bool Process::remove_watchpoint(uint64_t address) {
        auto it = std::find_if(watchpoints_.begin(), watchpoints_.end(),
                            [address](const Watchpoint& wp) { 
                                return wp.address == address; 
                            });
        
        if (it == watchpoints_.end()) {
            return false;
        }
        
        try {
            int slot = std::distance(watchpoints_.begin(), it);
            unsigned long dr_offset;
            switch (slot) {
                case 0: dr_offset = offsetof(user, u_debugreg[0]); break;
                case 1: dr_offset = offsetof(user, u_debugreg[1]); break;
                case 2: dr_offset = offsetof(user, u_debugreg[2]); break;
                case 3: dr_offset = offsetof(user, u_debugreg[3]); break;
                default: throw Exception("Invalid debug register slot");
            }
            
            if (ptrace(PTRACE_POKEUSER, current_thread_id, dr_offset, 0) == -1) {
                throw Exception("Failed to clear debug address register");
            }
            
            unsigned long dr7;
            if ((dr7 = ptrace(PTRACE_PEEKUSER, current_thread_id, 
                            offsetof(user, u_debugreg[7]), nullptr)) == -1) {
                throw Exception("Failed to read DR7 register");
            }
            
            dr7 &= ~(3UL << (slot * 2));
            
            if (ptrace(PTRACE_POKEUSER, current_thread_id, 
                    offsetof(user, u_debugreg[7]), dr7) == -1) {
                throw Exception("Failed to update DR7 register");
            }
            
            watchpoints_.erase(it);
            return true;
            
        } catch (const std::exception& e) {
            throw Exception("Failed to remove watchpoint: " + std::string(e.what()));
        }
    }
    std::vector<Watchpoint> Process::get_watchpoints() const {
        return watchpoints_;
    }

    bool Process::has_watchpoint_at(uint64_t address) const {
        return std::any_of(watchpoints_.begin(), watchpoints_.end(),
                        [address](const Watchpoint& wp) { 
                            return wp.address <= address && 
                                    address < wp.address + wp.size; 
                        });
    }

 
    void Process::switch_to_thread(pid_t tid) {
        std::string task_path = "/proc/" + std::to_string(m_pid) + "/task/" + std::to_string(tid);
        if (!std::filesystem::exists(task_path)) {
            throw Exception("Thread " + std::to_string(tid) + " does not exist");
        }
        std::ifstream status_file(task_path + "/status");
        if (!status_file.is_open()) {
            throw Exception("Cannot access thread " + std::to_string(tid) + " information");
        }
        std::string line;
        bool found_tgid = false;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 5) == "Tgid:") {
                std::istringstream iss(line);
                std::string label;
                pid_t tgid;
                iss >> label >> tgid;
                if (tgid != m_pid) {
                    throw Exception("Thread " + std::to_string(tid) + " does not belong to process " + std::to_string(m_pid));
                }
                found_tgid = true;
                break;
            }
        }
        if (!found_tgid) {
            throw Exception("Could not verify thread " + std::to_string(tid) + " belongs to process " + std::to_string(m_pid));
        }
        current_thread_id = tid;
    }
    pid_t Process::get_current_thread() const {
        return current_thread_id;
    }

    std::vector<pid_t> Process::get_thread_ids() const {
        std::vector<pid_t> thread_ids;
        std::string task_dir = "/proc/" + std::to_string(m_pid) + "/task";
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(task_dir)) {
                if (entry.is_directory()) {
                    std::string tid_str = entry.path().filename().string();
                    pid_t tid = std::stoi(tid_str);
                    thread_ids.push_back(tid);
                }
            }
        } catch (const std::exception& e) {
        }
        
        return thread_ids;
    }

    bool Process::is_valid_thread(pid_t tid) const {
        std::string task_path = "/proc/" + std::to_string(m_pid) + "/task/" + std::to_string(tid);
        return std::filesystem::exists(task_path);
    }
    
    user_fpregs_struct Process::get_fpu_registers() const {
        struct user_fpregs_struct fpregs;
        if (ptrace(PTRACE_GETFPREGS, current_thread_id, nullptr, &fpregs) == -1) {
            throw mx::Exception::error("Failed to get FPU registers");
        }
        return fpregs;
    }

    void Process::set_fpu_registers(const user_fpregs_struct& fpregs) {
        if (ptrace(PTRACE_SETFPREGS, current_thread_id, nullptr, &fpregs) == -1) {
            throw mx::Exception::error("Failed to set FPU registers");
        }
    }

    double Process::get_fpu_register(const std::string &text) {
        struct user_fpregs_struct fpregs = get_fpu_registers(); 
        if (text == "st0" || text == "st(0)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[0], 8);
            memcpy(&exponent, &fpregs.st_space[2], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            if (exp == -16383) return 0.0; 
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st1" || text == "st(1)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[4], 8);
            memcpy(&exponent, &fpregs.st_space[6], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st2" || text == "st(2)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[8], 8);
            memcpy(&exponent, &fpregs.st_space[10], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st3" || text == "st(3)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[12], 8);
            memcpy(&exponent, &fpregs.st_space[14], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st4" || text == "st(4)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[16], 8);
            memcpy(&exponent, &fpregs.st_space[18], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st5" || text == "st(5)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[20], 8);
            memcpy(&exponent, &fpregs.st_space[22], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st6" || text == "st(6)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[24], 8);
            memcpy(&exponent, &fpregs.st_space[26], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else if (text == "st7" || text == "st(7)") {
            uint64_t mantissa;
            uint16_t exponent;
            memcpy(&mantissa, &fpregs.st_space[28], 8);
            memcpy(&exponent, &fpregs.st_space[30], 2);
            
            int sign = (exponent >> 15) & 1;
            int exp = (exponent & 0x7FFF) - 16383;
            
            if (exp == -16383) return 0.0;
            
            double d = ldexp(1.0 + (double)mantissa / (1ULL << 63), exp);
            return sign ? -d : d;
        } else {
            throw mx::Exception("Unknown FPU register: " + text);
        }
     }

    void Process::set_fpu_register(const std::string &text, double value) {
        struct user_fpregs_struct fpregs = get_fpu_registers();
        union {
            double d;
            uint64_t u;
        } converter;
        converter.d = value;
        uint64_t mantissa = converter.u & 0x000FFFFFFFFFFFFF;
        int exponent = ((converter.u >> 52) & 0x7FF) - 1023;
        int sign = (converter.u >> 63) & 1;
        uint64_t ext_mantissa = mantissa << 11;
        uint16_t ext_exponent = (exponent + 16383) & 0x7FFF;
        if (sign) ext_exponent |= 0x8000;
        if (text == "st0" || text == "st(0)") {
            memcpy(&fpregs.st_space[0], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[2], &ext_exponent, 2);
        } else if (text == "st1" || text == "st(1)") {
            memcpy(&fpregs.st_space[4], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[6], &ext_exponent, 2);
        } else if (text == "st2" || text == "st(2)") {
            memcpy(&fpregs.st_space[8], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[10], &ext_exponent, 2);
        } else if (text == "st3" || text == "st(3)") {
            memcpy(&fpregs.st_space[12], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[14], &ext_exponent, 2);
        } else if (text == "st4" || text == "st(4)") {
            memcpy(&fpregs.st_space[16], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[18], &ext_exponent, 2);
        } else if (text == "st5" || text == "st(5)") {
            memcpy(&fpregs.st_space[20], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[22], &ext_exponent, 2);
        } else if (text == "st6" || text == "st(6)") {
            memcpy(&fpregs.st_space[24], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[26], &ext_exponent, 2);
        } else if (text == "st7" || text == "st(7)") {
            memcpy(&fpregs.st_space[28], &ext_mantissa, 8);
            memcpy(&fpregs.st_space[30], &ext_exponent, 2);
        } else {
            throw mx::Exception("Unknown FPU register: " + text);
        }

        set_fpu_registers(fpregs);
    }

    std::string Process::print_fpu_registers() {
        std::ostringstream out;
        try {
            struct user_fpregs_struct fpregs = get_fpu_registers();        
            out << "FPU Registers:" << std::endl;
            out << "==============" << std::endl;
            out << "Control Word: 0x" << std::hex << std::setfill('0') << std::setw(4) << fpregs.cwd << std::dec << std::endl;
            out << "Status Word:  0x" << std::hex << std::setfill('0') << std::setw(4) << fpregs.swd << std::dec << std::endl;
            out << "Tag Word:     0x" << std::hex << std::setfill('0') << std::setw(4) << fpregs.ftw << std::dec << std::endl;
            out << "Opcode:       0x" << std::hex << std::setfill('0') << std::setw(4) << fpregs.fop << std::dec << std::endl;
            out << "IP Offset:    0x" << std::hex << std::setfill('0') << std::setw(8) << fpregs.rip << std::dec << std::endl;
            out << "Data Offset:  0x" << std::hex << std::setfill('0') << std::setw(8) << fpregs.rdp << std::dec << std::endl;
            out << std::endl;
            out << "ST Registers:" << std::endl;
            out << "=============" << std::endl;
            for (int i = 0; i < 8; i++) {
                try {
                    std::string reg_name = "st" + std::to_string(i);
                    double value = get_fpu_register(reg_name);
                    out << "ST(" << i << "):  " << std::fixed << std::setprecision(15) << value << std::endl;
                } catch (const std::exception& e) {
                    out << "ST(" << i << "):  <error reading register>" << std::endl;
                }
            }
            out << std::endl;
            out << "Raw ST Space (hex):" << std::endl;
            out << "==================" << std::endl;
            for (int i = 0; i < 8; i++) {
                out << "ST(" << i << ") raw: ";
                for (int j = 0; j < 5; j++) {
                    int idx = i * 4 + j;
                    if (idx < 32) {
                        out << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)fpregs.st_space[idx] << " ";
                    }
                }
                out << std::dec << std::endl;
            }
            return out.str();
        } catch (const std::exception& e) {
            std::cerr << "Error reading FPU registers: " << e.what() << std::endl;
        }
        return out.str();
    }
    std::string Process::hex_dump(uint64_t address, uint64_t size) {
        std::ostringstream stream;
        if(is_running()) {
            auto v = read_memory(address, size);
            size_t bytes_per_line = 16;
            for (size_t i = 0; i < v.size(); i += bytes_per_line) {
                stream << format_hex64(address + i) << ": ";
                size_t line_bytes = std::min(bytes_per_line, v.size() - i);
                for (size_t j = 0; j < bytes_per_line; ++j) {
                    if (j < line_bytes) {
                        stream << format_hex8(v[i + j]) << " ";
                    } else {
                        stream << "     "; 
                    }
                }
                stream << " ";
                for (size_t j = 0; j < line_bytes; ++j) {
                    uint8_t c = v[i + j];
                    stream << (std::isprint(c) ? static_cast<char>(c) : '.');
                }
                stream << "\n";
            }
        } else {
            throw mx::Exception("Process is not running...");
        }
        return stream.str();
    }
}