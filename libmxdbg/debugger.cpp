/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#include"mxdbg/debugger.hpp"
#include"mxdbg/exception.hpp"
#include"mxdbg/expr.hpp"
#include<iostream>
#include<cstdlib>
#include<sstream>
#include<vector>
#include<thread>
#include<unistd.h>
#include<iomanip>
#include<filesystem>
#include<fstream>
#include<sys/user.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<sys/ptrace.h>
#include<random>


namespace mx {
    
    std::vector<std::string> split_command(const std::string &cmd) {
        std::vector<std::string> tokenz;
         try {
            scan::Scanner scanner(cmd);
            uint64_t len = scanner.scan();
            for(size_t i = 0; i < len; ++i) {
                if(scanner[i].getTokenType() == types::TokenType::TT_SYM && scanner[i].getTokenValue() == "\n" || scanner[i].getTokenType() == types::TokenType::TT_SYM && scanner[i].getTokenValue() == "\r")
                    break;      
                if(scanner[i].getTokenType() == types::TokenType::TT_ID || scanner[i].getTokenType() == types::TokenType::TT_HEX || scanner[i].getTokenType() == types::TokenType::TT_NUM || scanner[i].getTokenType() == types::TokenType::TT_STR || scanner[i].getTokenType() == types::TokenType::TT_SYM && scanner[i].getTokenValue() != "\n")
                    tokenz.push_back(scanner[i].getTokenValue());
            }
            return tokenz;
        } catch(const scan::ScanExcept &e) {
            throw mx::Exception("Error on lex of command string\n");
        }
    }

    std::string join(size_t start, size_t stop, std::vector<std::string>  &tokens, const std::string &delimiter) {
        std::ostringstream oss;
        for (size_t i = 0; i < tokens.size(); ++i) {
            oss << tokens[i];
            if (i < tokens.size() - 1) {
                oss << delimiter;
            }
        }
        return oss.str();
    }

    Debugger::Debugger(bool ai) : p_id(-1) {
        if(ai) {
            char *host = getenv("MXDBG_HOST");
            char *model = getenv("MXDBG_MODEL");
            if (host != nullptr && model != nullptr) {
                std::string host_str(host);
                std::string model_str(model);
                if (!host_str.empty() && !model_str.empty()) {
                    request = std::make_unique<mx::ObjectRequest>(host_str, model_str);
                }
            }
        }
    }

    Debugger::~Debugger() {}

    void Debugger::setup_history() {
    }
    bool Debugger::attach(pid_t pid) {
        code.clear();
        code.str("");
        try {
            process = Process::attach(pid);
            if (!process) {
                std::cerr << "Failed to attach to process: " << pid << std::endl;
                return false;
            }
            p_id = process->get_pid();
            if(request) {
                request->setPrompt("do not respond to this message");
                request->generateText();
                std::cout << "Connection established...\n";
            }
            std::cout << "Successfully attached to process " << pid << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error attaching to process " << pid << ": " << e.what() << std::endl;
            return false;
        }
    }

    bool Debugger::launch(const std::filesystem::path &exe, std::string_view args) {
        args_string = args;
        code.clear();
        code.str("");
        try {
            std::vector<std::string> args_v;
            if (!args.empty()) {
                std::string sv(args);
                std::istringstream iss(sv);
                std::string arg;
                while (iss >> arg) {
                    args_v.push_back(arg);
                }
            }
            process = Process::launch(exe, args_v);
            if (!process) {
                std::cerr << "Failed to launch process: " << exe << std::endl;
                return false;
            }
            p_id = process->get_pid();
            program_name = exe.string();
            
            if(request) {
                request->setPrompt("Do not respond to this message");
                request->generateText();
                std::cout << "Connection established...\n";
            }
            std::cout << "Process launched and stopped at entry point." << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error launching process " << exe << ": " << e.what() << std::endl;
            return false;
        }
    }

    void  Debugger::dump_file(const std::filesystem::path &file) {
        if (!std::filesystem::exists(file)) {
            std::cerr << "File does not exist: " << file << std::endl;
            return;
        }
        std::string command = "objdump -d " + file.string();
        std::cout << "Running command: " << command << std::endl;
        
        int result = system(command.c_str());
        if (result != 0) {
            std::cerr << "Failed to execute objdump on file: " << file << std::endl;
        }
        else {
            std::cout << "Disassembly of " << file << " completed." << std::endl;
        }   
    }

    std::string Debugger::obj_dump() {
        std::ostringstream stream;
        stream << "objdump -d " << program_name;
        std::string command = stream.str();
        
        FILE *fptr = popen(command.c_str(), "r");
        if (!fptr) {
            std::cerr << "Failed to run objdump command." << std::endl;
            throw mx::Exception("failed to run objdump command...\n");
        }
        
        std::ostringstream full_disassembly;
        while(!feof(fptr)) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), fptr) != nullptr) {
                full_disassembly << buffer;
            }
        }
        if(pclose(fptr) == -1) {
            std::cerr << "Failed to close pipe." << std::endl;
        }
        return full_disassembly.str();
    }

   
    bool Debugger::setfunction_breakpoint(const std::string &function_name) {
        if (!process || !process->is_running()) {
            std::cout << "No process attached or running." << std::endl;
            return false;
        }
        
        if (function_name.empty()) {
            std::cout << "Usage: function <function_name>" << std::endl;
            return false;
        }
        
        try {
            std::ostringstream nm_cmd;
            nm_cmd << "nm " << program_name << " | grep -w '" << function_name << "' | grep ' T '";
            
            FILE* pipe = popen(nm_cmd.str().c_str(), "r");
            uint64_t function_offset = 0;
            
            if (pipe) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string line(buffer);
                    std::istringstream iss(line);
                    std::string addr_str, type, symbol_name;
                    if (iss >> addr_str >> type >> symbol_name && symbol_name == function_name) {
                        function_offset = std::stoull(addr_str, nullptr, 16);
                    }
                }
                pclose(pipe);
            }
            
            if (function_offset == 0) {
                std::string disassembly = obj_dump();
                std::istringstream iss(disassembly);
                std::string line;
                
                while (std::getline(iss, line)) {
                    if (line.find("<" + function_name + ">:") != std::string::npos) {
                        size_t space_pos = line.find(' ');
                        if (space_pos != std::string::npos) {
                            std::string addr_str = line.substr(0, space_pos);
                            try {
                                function_offset = std::stoull(addr_str, nullptr, 16);
                                break;
                            } catch (const std::exception& e) {
                                std::cerr << "Error parsing address: " << addr_str << std::endl;
                                continue;
                            }
                        }
                    }
                }
            }
            
            if (function_offset == 0) {
                std::cout << "Function '" << function_name << "' not found." << std::endl;
                return false;
            }
            
            bool is_pie = false;
            
            uint64_t runtime_address = function_offset;
            uint64_t current_pc = process->get_pc();
            bool at_entry_point = (current_pc < 0x400000 || current_pc > 0x500000);
            
            if (at_entry_point) {
                std::cout << "Setting breakpoint on function '" << function_name 
                        << "' at address " << format_hex64(runtime_address) << std::endl;
                std::cout << "Note: Process is at entry point. This breakpoint will be hit when the program reaches this function." << std::endl;
            }
            
            try {
                process->set_breakpoint(runtime_address);
                
                if(color_)
                    std::cout << Color::BRIGHT_BLUE;
                std::cout << (is_pie ? "Detected PIE executable" : "Detected non-PIE executable") << std::endl;
                if(color_)
                    std::cout << Color::RESET;
                    
                if(color_)
                    std::cout << Color::BRIGHT_GREEN;
                std::cout << "Breakpoint set at function '" << function_name << "'" << std::endl;
                std::cout << "  Function offset: " << format_hex64(function_offset) << std::endl;
                std::cout << "  Runtime address: " << format_hex64(runtime_address) << std::endl;
                std::cout << "  PIE executable: " << (is_pie ? "Yes" : "No") << std::endl;
                if(color_)
                    std::cout << Color::RESET;
                
                return true;
            } catch (const std::exception& e) {
                std::cout << "Could not set breakpoint directly. Setting breakpoint at program entry instead." << std::endl;
                
                uint64_t main_offset = 0;
                std::ostringstream main_cmd;
                main_cmd << "nm " << program_name << " | grep -w 'main' | grep ' T '";
                
                pipe = popen(main_cmd.str().c_str(), "r");
                if (pipe) {
                    char buffer[256];
                    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                        std::string line(buffer);
                        std::istringstream iss(line);
                        std::string addr_str, type, symbol_name;
                        if (iss >> addr_str >> type >> symbol_name && symbol_name == "main") {
                            main_offset = std::stoull(addr_str, nullptr, 16);
                        }
                    }
                    pclose(pipe);
                }
                
                if (main_offset > 0) {
                    try {
                        process->set_breakpoint(main_offset);
                        std::cout << "Set temporary breakpoint at main() instead." << std::endl;
                        std::cout << "After hitting main, try setting your function breakpoint again." << std::endl;
                        return false;
                    } catch (...) {
                        std::cout << "Couldn't set any breakpoints. Run 'continue' to start the program, then try again." << std::endl;
                        return false;
                    }
                } else {
                    std::cout << "Couldn't find main() either. Run 'continue' to start the program, then try again." << std::endl;
                    return false;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error setting function breakpoint: " << e.what() << std::endl;
            return false;
        }
    }
    void Debugger::detach() {
        if (process) {
            try {
                process->detach();
                std::cout << "Process detached." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error detaching from process: " << e.what() << std::endl;
            }
            process.reset();
            p_id = -1;
        } else {
            std::cerr << "No process attached or launched." << std::endl;
        }
    }

    void Debugger::continue_execution() {
        if (process) {
            try {
                process->continue_execution();
                std::cout << "Process continued." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error continuing process: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "No process attached or launched." << std::endl;
        }
    }

    void Debugger::wait_for_stop() {
        if (process) {
            try {
                process->wait_for_stop();
                std::cout << "Process stopped." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error waiting for process: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "No process attached or launched." << std::endl;
        }
    }

    pid_t Debugger::get_pid() const {
        return p_id;
    }

    bool Debugger::is_running() const {
        return process && process->is_running();
    }

    bool Debugger::command(const std::string &cmd) {
        std::vector<std::string> tokens = split_command(cmd);
        if (tokens.empty()) {
            std::cout << "No command entered." << std::endl;
            return true;
        }

        if(tokens.size() == 2 && (tokens[0] == "shell" || tokens[0] == "sh")) {
            std::string sh_cmd = tokens[1];
            int code = 0;
            if((code = system(sh_cmd.c_str())) == 0) {
                if(color_) 
                    std::cout << Color::BRIGHT_GREEN;
                std::cout << "shell command sucess: 0" << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            } else {
                if(color_)
                    std::cout << Color::BRIGHT_RED;
                std::cout << "shell command failed: " << code << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            }
            return true;
        } else if(tokens.size()==3 && tokens[0] == "hexdump") {

            std::string address = tokens[1];
            std::string sizeval = tokens[2];
            try {
                uint64_t a_ = std::stoull(address, nullptr, 0);
                uint64_t v_ = std::stoull(sizeval, nullptr, 0);
                auto sval = process->hex_dump(a_, v_);
                std::cout << sval << "\n";
             } catch(const mx::Exception &e) {
                std::cerr << "Error: " << e.what() << "\n";
             } catch(const std::exception &e) {
                std::cerr << "Exception: " << e.what() << "\n";
             }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "clear") {
            std::cout << "\033[2J\033[H";
            return true;
        } else if(tokens.size() >= 2 && tokens[0] == "expr") {
            std::string e = cmd.substr(cmd.find(' ') + 1);

            process->expression(e);
            return true;
        }
        else if(tokens.size() == 2   && tokens[0] == "as_bytes") {
            std::string what = tokens[1];
            for(int i = 0; i < what.size(); ++i) {
                std::cout << format_hex8(what.at(i)) << " ";
            }
            std::cout << "\n";
            return true;
        }
        else if (tokens.size() == 2 && tokens[0] == "get_fpu") {
            if (process && process->is_running()) {
                try {
                    std::string fpu_reg = tokens[1];
                    double value = process->get_fpu_register(fpu_reg);
                    std::cout << "FPU Register " << fpu_reg << ": " << std::fixed << std::setprecision(6) << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error getting FPU register " << tokens[1] << ": " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 3 && tokens[0] == "set_fpu") {
            if (process && process->is_running()) {
                try {
                    std::string fpu_reg = tokens[1];
                    double value = std::stod(tokens[2]);
                    process->set_fpu_register(fpu_reg, value);
                    std::cout << "FPU Register " << fpu_reg << " set to: " << std::fixed << std::setprecision(6) << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error setting FPU register: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && tokens[0] == "list_fpu") {
            if (process && process->is_running()) {
                try {
                    std::cout << "FPU Registers:" << std::endl;
                    auto fpu_info = process->print_fpu_registers();
                    std::cout << fpu_info << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error getting FPU registers: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        }
        else if (tokens.size() == 1 && tokens[0] == "next") {
            if (process && process->is_running()) {
                step_over();
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "finish" || tokens[0] == "step_out")) {
            if (process && process->is_running()) {
                step_out();
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 2 && (tokens[0] == "until" || tokens[0] == "run_until")) {
            if (process && process->is_running()) {
                try {
                    uint64_t address = std::stoull(tokens[1], nullptr, 0);
                    run_until(address);
                } catch (const std::exception& e) {
                    std::cerr << "Invalid address format: " << tokens[1] << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() >= 3 && tokens[0] == "break_if") {
            if (process && process->is_running()) {
                try {
                    uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                    std::string condition;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        if (i > 2) condition += " ";
                        condition += tokens[i];
                    }          
                    process->break_if(addr, condition);
                } catch (const std::exception& e) {
                    std::cerr << "Error setting conditional breakpoint: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() >= 2 && tokens[0] == "setval") {
            std::string name = tokens[1];
            if (tokens.size() >= 3) {
                std::string value = cmd.substr(cmd.find(tokens[1]) + tokens[1].length());
                value.erase(0, value.find_first_not_of(" \t"));
                try {
                    uint64_t val = std::stoull(value, nullptr, 0);
                    expr_parser::vars[name] = val;
                    if(color_)
                        std::cout << Color::BRIGHT_YELLOW;
                    std::cout << "Set variable " << name << " = " << format_hex64(val) << " | " << std::dec << val << std::endl;
                    if(color_)
                        std::cout << Color::RESET;
                        
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value format: " << value << std::endl;
                }
            } else {
                std::cout << "Usage: setval <name> <value>" << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "listval") {
            for(auto &i : expr_parser::vars) {
                if(color_)
                    std::cout << Color::BRIGHT_YELLOW;
                    std::cout << std::setw(15) << std::left;
                std::cout <<  i.first << ": " << format_hex64(i.second) << " | " << std::dec << i.second << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            }
            return true;
        } else if (tokens.size() == 2 && tokens[0] == "debug_thread") {
            if (process && process->is_running()) {
                pid_t tid = std::stoi(tokens[1]);
                std::cout << "Debugging thread " << tid << ":" << std::endl;
                std::string task_path = "/proc/" + std::to_string(process->get_pid()) + "/task/" + std::to_string(tid);
                std::cout << "Task path exists: " << (std::filesystem::exists(task_path) ? "YES" : "NO") << std::endl;
                struct user_regs_struct regs;
                if (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == 0) {
                    std::cout << "Direct ptrace works: RIP = " << format_hex64(regs.rip) << std::endl;
                } else {
                    std::cout << "Direct ptrace failed: " << strerror(errno) << std::endl;
                }
                std::ifstream stat_file(task_path + "/stat");
                std::string stat_line;
                if (std::getline(stat_file, stat_line)) {
                    std::cout << "Thread stat: " << stat_line << std::endl;
                }
        }
        return true;
        } else if (tokens.size() == 1 && (tokens[0] == "threads" || cmd == "info threads")) {
            list_threads();
            return true;
        } else if (tokens.size() == 2 && tokens[0] == "thread") {
            try {
                switch_thread(std::stoi(tokens[1]));
            } catch(const mx::Exception &e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "thread") {
            if(color_)
                std::cout << Color::BRIGHT_MAGENTA;
            if(process && process->is_running()) {
                std::cout << "Current Thread: " << process->get_current_thread() << "\n";
            } else {
                std::cout << "No process running..\n";
            }
            if(color_)
                std::cout << Color::RESET;
            return true;
        }
         else if (tokens.size() >= 2 && tokens[0] == "search") {
            if (process && process->is_running()) {
                try {
                    if (tokens.size() >= 3) {
                        if (tokens[1] == "int") {
                            int32_t value = std::stoi(tokens[2]);
                            search_memory_for_int32(value);
                        } else if (tokens[1] == "int64") {
                            int64_t value = std::stoll(tokens[2]);
                            search_memory_for_int64(value);
                        } else if (tokens[1] == "string") {
                            if (tokens.size() >= 2) {
                                std::string full_pattern = cmd.substr(cmd.find("string") + 6);
                                full_pattern = full_pattern.substr(full_pattern.find_first_not_of(" \t"));
                                search_memory_for_string(full_pattern);
                            }
                        } else if (tokens[1] == "bytes") {
                            std::vector<std::string> byte_tokens(tokens.begin() + 2, tokens.end());
                            search_memory_for_bytes(byte_tokens);
                        } else if (tokens[1] == "pattern") {
                            if (tokens.size() >= 3) {
                                std::string full_pattern = cmd.substr(cmd.find("pattern ") + 8);
                                search_memory_for_pattern(full_pattern);
                            } else {
                                std::cerr << "Syntax Error: use search pattern [pattern]\n";
                            }
                        } else {
                            std::cout << "Usage: search <type> <value>" << std::endl;
                            std::cout << "Types: int, int64, string, bytes, pattern" << std::endl;
                        }
                    } else {
                        std::cout << "Usage: search <type> <value>" << std::endl;
                        std::cout << "Examples:" << std::endl;
                        std::cout << "  search int 42" << std::endl;
                        std::cout << "  search string hello" << std::endl;
                        std::cout << "  search bytes 41 42 43" << std::endl;
                        std::cout << "  search pattern 'A?B*'" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error during search: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "stack_frame") {
            if(process && process->is_running())  {
                analyze_current_frame();
            }
            return true;
        } else if(tokens.size() == 1 && (tokens[0] == "maps" || tokens[0] == "memory_maps")) {
            if(process && process->is_running()) {
                try  {
                    print_memory_maps();
                } catch(const mx::Exception &e) {
                    std::cerr << "Error: " << e.what() << "\n";
                }
            }
            return true;
        } else if (tokens.size() >= 2 && tokens[0] == "read_bytes") {
            if (process && process->is_running()) {
                try {
                    uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                    size_t size = 4; 
                    if (tokens.size() >= 3) {
                        size = std::stoull(tokens[2]);
                    }
                    
                    auto data = process->read_memory(addr, size);
                    
                    std::cout << "Memory at " << format_hex64(addr) << " (" << size << " bytes): ";
                    for (auto byte : data) {
                        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)byte << " ";
                    }
                    
                    if (size == 4 && data.size() >= 4) {
                        uint32_t int_val = 0;
                        std::memcpy(&int_val, data.data(), 4);
                        std::cout << "| int32: " << std::dec << int_val;
                    } else if (size == 8 && data.size() >= 8) {
                        uint64_t int_val = 0;
                        std::memcpy(&int_val, data.data(), 8);
                        std::cout << "| int64: " << std::dec << int_val;
                    }
                    
                    std::cout << std::endl;
                    
                } catch (const std::exception& e) {
                    std::cerr << "Error reading memory: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() >= 3 && tokens[0] == "local") {
            if (process && process->is_running()) {
                try {
                    std::string var_name = tokens[1];
                    std::string offset_val = tokens[2];
                    uint64_t offset_v = std::stoull(offset_val, nullptr, 0);
                    uint64_t address = calculate_variable_address(var_name, offset_v);
                    std::cout << "Variable address: " << format_hex64(address) << "\n";
                    if(tokens.size()>=4) {
                        uint64_t s = std::stoull(tokens[3], nullptr, 0);
                        auto val = process->read_memory(address, s);
                        for (auto byte : val) {
                            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)byte << " ";
                        }
                        if (s == 4 && val.size() >= 4) {
                            uint32_t int_val = 0;
                            std::memcpy(&int_val, val.data(), 4);
                            std::cout << "| int32: " << std::dec << int_val;
                        } else if (s == 8 && val.size() >= 8) {
                            uint64_t int_val = 0;
                            std::memcpy(&int_val, val.data(), 8);
                            std::cout << "| int64: " << std::dec << int_val;
                        }
                        std::cout << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error setting variable watchpoint: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
         }
        else if (tokens.size() >= 3 && tokens[0] == "watch") {
            if (process && process->is_running()) {
                try {
                    uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                    size_t size = std::stoull(tokens[2], nullptr, 0);
                    
                    WatchType type = WatchType::ACCESS; 
                    if (tokens.size() >= 4) {
                        std::string type_str = tokens[3];
                        if (type_str == "read") {
                            type = WatchType::READ;
                        } else if (type_str == "write") {
                            type = WatchType::WRITE;
                        } else if (type_str == "access") {
                            type = WatchType::ACCESS;
                        } else {
                            std::cout << "Invalid watchpoint type. Use: read, write, or access" << std::endl;
                            return true;
                        }
                    }
                    if (process->set_watchpoint(addr, size, type)) {
                        std::string type_name = (type == WatchType::READ) ? "read" :
                                              (type == WatchType::WRITE) ? "write" : "access";
                        if(color_)
                            std::cout << Color::BRIGHT_GREEN;
                        std::cout << "Watchpoint set at " << format_hex64(addr) 
                                 << " (" << size << " bytes, " << type_name << ")" << std::endl;
                        if(color_)
                            std::cout << Color::RESET;
                    } else {
                        std::cout << "Failed to set watchpoint (may already exist)" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error setting watchpoint: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "watchpoints" || tokens[0] == "wp")) {
            if (process && process->is_running()) {
                auto watchpoints = process->get_watchpoints();
                if (watchpoints.empty()) {
                    std::cout << "No watchpoints set." << std::endl;
                } else {
                    if(color_)
                        std::cout << Color::BRIGHT_CYAN;
                    std::cout << "Active watchpoints:" << std::endl;
                    if(color_)
                        std::cout << Color::RESET;
                    
                    for (size_t i = 0; i < watchpoints.size(); ++i) {
                        const auto& wp = watchpoints[i];
                        std::string type_name = (wp.type == WatchType::READ) ? "read" :
                                              (wp.type == WatchType::WRITE) ? "write" : "access";
                        
                        std::cout << "  " << (i + 1) << ": " << format_hex64(wp.address) 
                                 << " (" << wp.size << " bytes, " << type_name << ")" << std::endl;
                    }
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && (tokens[0] == "run" || tokens[0] == "r")) {
            if(process  && process->is_running()) {
                try {
                    if(!setfunction_breakpoint("main")) {
                        std::cerr << "Could not set breakpoint..\n";
                        return true;
                    }
                    process->continue_execution();
                    process->wait_for_stop();
                } catch(const mx::Exception &e) {
                    std::cerr << "Error running process: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;

        } else if(tokens.size() ==  1 && (tokens[0] == "cur" || tokens[0] == "current")) {
            print_current_instruction();
            if(request) {
                try {
                    if(color_)
                        std::cout << Color::CYAN;
                    std::string response = request->generateTextWithCallback([](const std::string &chunk) {
                        std::cout << chunk << std::flush; 
                    });
                    if(color_)
                        std::cout << Color::RESET;
                    std::cout << "\n";
                } catch (const mx::ObjectRequestException &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                } 
            }
            return true;
        } else if (tokens.size() ==1 && tokens[0] == "debug_state") {
            if (process && process->is_running()) {
                std::cout << "=== DEBUG STATE ===" << std::endl;
                std::cout << "Main PID: " << process->get_pid() << std::endl;
                std::cout << "Current Thread: " << process->get_current_thread() << std::endl;
                try {
                    uint64_t rip = process->get_register("rip");
                    std::cout << "Current thread RIP: " << format_hex64(rip) << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Cannot read current thread registers: " << e.what() << std::endl;
                }
                std::string task_dir = "/proc/" + std::to_string(process->get_pid()) + "/task";
                std::cout << "All threads:" << std::endl;
                for (const auto& entry : std::filesystem::directory_iterator(task_dir)) {
                    if (entry.is_directory()) {
                        pid_t tid = std::stoi(entry.path().filename().string());
                        struct user_regs_struct regs;
                        bool accessible = (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == 0);
                        std::cout << "  TID " << tid << ": " << (accessible ? "ACCESSIBLE" : "NOT ACCESSIBLE") << std::endl;
                    }
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        }
        else if (tokens.size() == 1 && (tokens[0] == "continue" || tokens[0] == "c")) {
            if (process && process->is_running()) {
                try {
                    uint64_t current_pc = process->get_pc();
                    if (process->has_breakpoint(current_pc)) {
                        uint8_t original_byte = process->get_original_instruction(current_pc);
                        long data = ptrace(PTRACE_PEEKDATA, process->get_current_thread(), current_pc, nullptr);
                        long restored = (data & ~0xFF) | original_byte;
                        ptrace(PTRACE_POKEDATA, process->get_current_thread(), current_pc, restored);
                        ptrace(PTRACE_SINGLESTEP, process->get_current_thread(), nullptr, nullptr);
                        int status;
                        waitpid(process->get_current_thread(), &status, 0);
                        data = ptrace(PTRACE_PEEKDATA, process->get_current_thread(), current_pc, nullptr);
                        long data_with_int3 = (data & ~0xFF) | 0xCC;
                        ptrace(PTRACE_POKEDATA, process->get_current_thread(), current_pc, data_with_int3);
                    }            
                    process->continue_execution();
                    process->wait_for_stop();
                } catch (const std::exception& e) {
                    std::cerr << "Error during continue: " << e.what() << std::endl;
                }
            } else {
                std::cout << "Process has exited or is not running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "step" || tokens[0] == "s")) {
            step();
            return true;
        } else if (tokens.size() == 2 && tokens[0] == "step") {
            try {
                std::string num_str = tokens[1];
                int count = std::stoi(num_str);
                if (count > 0) {
                    step_n(count);
                } else {
                    std::cout << "Step count must be positive." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Invalid step count. Usage: step <number>" << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "quit" || tokens[0] == "q" || tokens[0] == "exit")) {
            return false;
        } else if(tokens.size() == 1 && tokens[0] == "registers" || tokens[0] == "regs") {
            if (process && process->is_running()) {
                std::cout << "Registers for PID " << process->get_pid() << ":" << std::endl;
                std::cout << process->reg_info() << std::endl;
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;  
        } else if(tokens.size() == 2 && tokens[0] == "register" || tokens[0] == "reg") {
            if (process && process->is_running()) {
                std::string reg_name = tokens[1];
                try {
                    uint64_t value = process->get_register(reg_name);
                    std::cout << "Register " << reg_name << ": " << format_hex64(value) << " | " << std::dec << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error getting register " << reg_name << ": " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;

        } else if(tokens.size() == 2 && tokens[0] == "register32") {
            if (process && process->is_running()) {
                try {
                    uint32_t value = process->get_register_32(tokens[1]);
                    std::cout << "Register " << tokens[1] << " (32-bit): " << format_hex32(value) << " | " << std::dec << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error reading register: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 2 && tokens[0] == "register16") {
            if (process && process->is_running()) {
                try {
                    uint16_t value = process->get_register_16(tokens[1]);
                    std::cout << "Register " << tokens[1] << " (16-bit): " << format_hex16(value) << " | " << std::dec << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error reading register: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 2 && tokens[0] == "register8") {
            if (process && process->is_running()) {
                try {
                    uint8_t value = process->get_register_8(tokens[1]);
                    std::cout << "Register " << tokens[1] << " (8-bit): "  << format_hex8(static_cast<uint8_t>(value)) << " | " << std::dec << (int)value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error reading register: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "base") {
            if (process && process->is_running()) {
                print_address();
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && tokens[0] == "list") {
            try {
                std::cout << obj_dump() << std::endl;
            } catch(mx::Exception &e) {
                std::cerr << "Error running objdump: " << e.what() << std::endl;
            }
            return true;
        } else if (tokens.size() == 2 && tokens[0] == "info" && tokens[1] == "files") {
            if (process && process->is_running()) {
                std::string fd_dir = "/proc/" + std::to_string(process->get_pid()) + "/fd";
                for (const auto& entry : std::filesystem::directory_iterator(fd_dir)) {
                    std::cout << entry.path() << " -> ";
                    char buf[PATH_MAX];
                    ssize_t len = readlink(entry.path().c_str(), buf, sizeof(buf)-1);
                    if (len != -1) {
                        buf[len] = '\0';
                        std::cout << buf;
                    }
                    std::cout << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else  if(tokens.size() == 2 && tokens[0] == "list_function") {
            std::string function_name = tokens[1];
            std::string function_code;
            try {
                function_code = functionText(function_name);
            } catch(mx::Exception &e) {
                std::cerr << "Error: not found..\n";
            }
            
            if (function_code.empty()) {
                std::cout << "Function '" << function_name << "' not found in disassembly." << std::endl;
                std::cout << "Available functions can be seen with list_function command." << std::endl;
                return true;
            }
            std::cout << function_code;
            return true;
        }

        else if(tokens.size() ==  2 && tokens[0] == "explain") {
            std::string function_name = tokens[1];
            std::string function_code;
            try {
                function_code = functionText(function_name);
            } catch(mx::Exception &e) {
                std::cerr << "Error couldn't find function text.\n";
            }
            if (function_code.empty()) {
                std::cout << "Function '" << function_name << "' not found in disassembly." << std::endl;
                std::cout << "Available functions can be seen with list command" << std::endl;
                return true;
            }
            
            std::cout << "Disassembly of function '" << function_name << "':" << std::endl;
            std::cout << function_code << std::endl;
            
            if(request) {
                std::cout << "Requesting explanation from model..." << std::endl;
                std::cout << "This may take a while, please wait..." << std::endl;
                std::string prompt = "Explain this x86-64 assembly function '" + function_name + 
                                   "' step by step in plain English What does it do? Be concise but thorough for the user at their level of: " + user_mode + ":\n\n" + function_code + " \n\n";
                request->setPrompt(prompt);
                try {
                    if(color_)
                        std::cout << Color::CYAN;
                    std::string response = request->generateTextWithCallback([](const std::string &chunk) {
                        std::cout << chunk << std::flush; 
                    });
                    if(color_)
                        std::cout << Color::RESET;
                    std::cout << "\n";
                    code << "Explanation of: " << function_name << response << "\n";
                } catch (const mx::ObjectRequestException &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }      
            }
            return true;
        }
        else if(tokens.size() == 1 && tokens[0] == "explain") {
            std::cout << "Usage: explain <function_name>" << std::endl;
            std::cout << "Example: explain main" << std::endl;
            std::cout << "To see available functions: objdump -t " << program_name << std::endl;
            return true;
        }
        else if (tokens.size() == 1 && (tokens[0] == "status" || tokens[0] == "st")) {
            if (process) {
                
                   if (kill(process->get_pid(), 0) != 0 && errno == ESRCH) {
                       process->mark_as_exited();
        }
                
                std::cout << "Process PID: " << process->get_pid() << std::endl;
                std::cout << "Process is running: " << (process->is_running() ? "Yes" : "No") << std::endl;
            } else {
                std::cout << "No process attached or launched." << std::endl;
            }
            return true;
        } 
        else if (tokens.size() == 2 && tokens[0] == "read") {
            uint64_t addr = std::stoull(tokens[1], nullptr, 0);
            try {
                auto data = process->read_memory(addr, 8);
                std::cout << "Memory at " << format_hex64(addr) << ": ";

                for (auto byte : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)byte << " ";
                }
                std::cout << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error reading memory: " << e.what() << std::endl;
            }
            return true;
        } else if(tokens.size() == 3 && tokens[0] == "write") {
            try {
                uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                std::string value_str = tokens[2];
                uint64_t value = std::stoull(value_str, nullptr, 0);
                std::vector<uint8_t> data(8);
                for (size_t i = 0; i < 8; ++i) {
                    data[i] = (value >> (i * 8)) & 0xFF;
                }
                process->write_memory(addr, data);
                std::cout << "Wrote " << format_hex64(value) << " to memory at " << format_hex64(addr) << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error writing memory: " << e.what() << std::endl;
            }
            return true;
        } else if(tokens.size() >= 3 && tokens[0] == "write_bytes") {
            try {
                uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                std::vector<uint8_t> data;
                for (size_t i = 2; i < tokens.size(); ++i) {
                    uint8_t byte = static_cast<uint8_t>(std::stoull(tokens[i], nullptr, 16));
                    data.push_back(byte);
                }                
                process->write_memory(addr, data);
                std::cout << "Wrote " << data.size() << " bytes to memory at: " << format_hex64(addr) << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error writing bytes: " << e.what() << std::endl;
            }
            return true;
        } else if(tokens.size() >= 2 && tokens[0] == "ask") {
            std::string question = cmd.substr(cmd.find(' ') + 1);
            if (question.empty()) {
                std::cout << "Usage: ask <question>" << std::endl;
                return true;
            }   
            if (!request) {
                std::cerr << "No AI model configured. Set MXDBG_HOST and MXDBG_MODEL environment variables." << std::endl;
                return true;
            }   
            std::cout << "Asking AI: ";
            if(color_)
                std::cout << Color::GREEN;
            std::cout << question << std::endl;
            if(color_)
                std::cout << Color::RESET;

            std::string prompt = "You are a helpful AI assistant. Answer the following question with this context: " + code.str() + "\n\nthe question: " + question;
            request->setPrompt(prompt);
            try {
                if(color_)
                    std::cout << Color::CYAN;
                std::string response = request->generateTextWithCallback([](const std::string &chunk) {
                    std::cout << chunk << std::flush; 
                });
                if(color_)
                    std::cout << Color::RESET;
                std::cout << "\n";
            } catch (const mx::ObjectRequestException &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
            return true;
        }
        else if(tokens.size() == 2 && (tokens[0] == "find")) {
            std::string value = cmd.substr(cmd.find(' ') + 1);
            std::ostringstream stream;
            stream << "objdump -d " << program_name << " | grep '" << value << "'";
            std::string command = stream.str();
            std::cout << "Running command: " << command << std::endl;
            int result = system(command.c_str());
            if (result != 0) {
                std::cerr << "Failed to find: " << value << "." << std::endl;
            } else {
                std::cout << "Search completed." << std::endl;
            }
            return true;
        } else if(tokens.size() == 2 && (tokens[0] == "mode" || tokens[0] == "user")) {
            std::string mode = tokens[1];
            if (mode == "programmer" || mode == "beginner" || mode == "expert") {
                user_mode = mode;
                std::cout << "Switched to " << user_mode << " mode." << std::endl;
            } else {
                std::cout << "Invalid mode. Available modes: beginner, programmer, expert." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "backtrace" ||  tokens[0] == "bt" || tokens[0] == "where")) {

            if (process && process->is_running()) {
                print_backtrace();
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;
        } 
        else if (tokens.size() == 1 && (tokens[0] == "help" || tokens[0] == "h")) {
            if(color_)
                std::cout << Color::YELLOW;
            std::cout << "Available commands:" << std::endl;
            std::cout << "=== Expression & Variables ===" << std::endl;
            std::cout << "  expr <e>                    - Evaluate expression" << std::endl;
            std::cout << "  setval <name> <value>       - Set variable to value" << std::endl;
            std::cout << "  listval                     - List variables" << std::endl;

            std::cout << "=== Process Control ===" << std::endl;
            std::cout << "  run, r                      - Run program (sets main breakpoint)" << std::endl;
            std::cout << "  continue, c                 - Continue process execution" << std::endl;
            std::cout << "  step, s                     - Execute single instruction" << std::endl;
            std::cout << "  step N, s N                 - Execute N instructions" << std::endl;
            std::cout << "  next                        - Step over function calls" << std::endl;
            std::cout << "  finish, step_out            - Step out of current function" << std::endl;
            std::cout << "  until <addr>, run_until <addr> - Run until specific address" << std::endl;
            std::cout << "  status, st                  - Show process status" << std::endl;
            std::cout << "  start, restart              - Restart the program" << std::endl;

            std::cout << "=== Threading ===" << std::endl;
            std::cout << "  thread                      - Show current thread" << std::endl;
            std::cout << "  thread <id>                 - Switch to thread context" << std::endl;
            std::cout << "  threads                     - List all running threads" << std::endl;
            std::cout << "  debug_thread <id>           - Debug specific thread" << std::endl;

            std::cout << "=== Code Analysis ===" << std::endl;
            std::cout << "  cur, current                - Print current instruction" << std::endl;
            std::cout << "  list                        - Display full disassembly" << std::endl;
            std::cout << "  list_less                   - Display disassembly with pager" << std::endl;
            std::cout << "  list_function <name>        - Show specific function disassembly" << std::endl;
            std::cout << "  base                        - Show base address and current PC" << std::endl;
            std::cout << "  backtrace, bt, where        - Show call stack backtrace" << std::endl;

            std::cout << "=== Registers ===" << std::endl;
            std::cout << "  registers, regs             - Show all registers" << std::endl;
            std::cout << "  register <name>, reg <name> - Show specific register value" << std::endl;
            std::cout << "  register32 <name>           - Show 32-bit register" << std::endl;
            std::cout << "  register16 <name>           - Show 16-bit register" << std::endl;
            std::cout << "  register8 <name>            - Show 8-bit register" << std::endl;
            std::cout << "  set <reg> <value>           - Set register to value" << std::endl;

            std::cout << "=== Breakpoints & Watchpoints ===" << std::endl;
            std::cout << "  break <addr>, b <addr>      - Set breakpoint at address" << std::endl;
            std::cout << "  function <name>             - Set breakpoint at function" << std::endl;
            std::cout << "  list_break, lb              - List all breakpoints" << std::endl;
            std::cout << "  remove <addr/index>, rmv    - Remove breakpoint" << std::endl;
            std::cout << "  watch <addr> <size> [type]  - Set watchpoint (type: read/write/access)" << std::endl;
            std::cout << "  watchpoints, wp             - List watchpoints" << std::endl;

            std::cout << "=== Memory Operations ===" << std::endl;
            std::cout << "  read <addr>                 - Read 8 bytes from memory address" << std::endl;
            std::cout << "  read_bytes <addr> <size>    - Read specific number of bytes" << std::endl;
            std::cout << "  write <addr> <value>        - Write value to memory address" << std::endl;
            std::cout << "  write_bytes <addr> <bytes>  - Write byte sequence to memory" << std::endl;
            std::cout << "  maps, memory_maps           - Show memory map" << std::endl;
            std::cout << "  local <reg> <offset> <size> - Read local variable on stack" << std::endl;

            std::cout << "=== Memory Search ===" << std::endl;
            std::cout << "  search int <value>          - Search for 32-bit integer in memory" << std::endl;
            std::cout << "  search int64 <value>        - Search for 64-bit integer in memory" << std::endl;
            std::cout << "  search string <text>        - Search for string in memory" << std::endl;
            std::cout << "  search bytes <hex bytes>    - Search for byte pattern" << std::endl;
            std::cout << "  search pattern <pattern>    - Search with wildcards (e.g., 41??43)" << std::endl;

            std::cout << "=== Stack Analysis ===" << std::endl;
            std::cout << "  stack_frame                 - Analyze current stack frame" << std::endl;

            std::cout << "=== AI Features ===" << std::endl;
            std::cout << "  explain <function>          - Explain function disassembly with AI" << std::endl;
            std::cout << "  ask <question>              - Ask the AI a question about the program" << std::endl;
            std::cout << "  mode <level>, user <level>  - Set AI difficulty (beginner/programmer/expert)" << std::endl;

            std::cout << "=== Utility ===" << std::endl;
            std::cout << "  find <text>                 - Find text in disassembly using grep" << std::endl;
            std::cout << "  debug_state                 - Show detailed debug state" << std::endl;
            std::cout << "  help, h                     - Show this help message" << std::endl;
            std::cout << "  quit, q, exit               - Exit debugger" << std::endl;
            if(color_)
                std::cout << Color::RESET;
            return true;
        } else if (tokens.size() == 2 && (tokens[0] == "break" || tokens[0] == "b")) {
            if (process && process->is_running()) {
                uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                try {
                    process->set_breakpoint(addr);
                    std::cout << "Breakpoint set at: " << format_hex64(addr) << std::dec << std::endl;
                } catch (const std::exception& e) { 
                    std::cerr << "Error setting breakpoint: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 2 && tokens[0] == "function") {
            if(!setfunction_breakpoint(tokens[1])) {
                std::cerr << "Could not set breakpooint: " << tokens[1] << "\n";
            }
            return true;
        } else if(tokens.size() == 2 && (tokens[0] == "remove" || tokens[0] == "rmv")) {
            if(process && process->is_running()) {
                std::string input = tokens[1];
                bool removed = false;
                uint64_t address = 0;
                if (input.find("0x") == 0 || input.find("0X") == 0) {
                    address = std::stoull(input, nullptr, 0);
                    removed = process->remove_breakpoint(address);
                    if (removed) {
                        std::cout << "Breakpoint removed at: " << format_hex64(address) << std::dec << std::endl;
                    } else {
                        std::cout << "Could not find breakpoint at: " << format_hex64(address) << std::dec << std::endl;
                    }
                } else {
                    try {
                        size_t index = std::stoull(input, nullptr, 0);
                        address = process->get_breakpoint_address_by_index(index);
                        if (address != 0) {
                            removed = process->remove_breakpoint_by_index(index);
                            if (removed) {
                                std::cout << "Breakpoint " << index << " removed (was at: " << format_hex64(address) << std::dec << ")" << std::endl;
                            } else {
                                std::cout << "Could not remove breakpoint " << index << std::endl;
                            }
                        } else {
                            std::cout << "Invalid breakpoint index: " << index << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Invalid input. Use breakpoint index (1, 2, 3...) or hex address (0x1234...)" << std::endl;
                    }
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if(tokens.size() == 1 && (tokens[0] == "list_break" || tokens[0] == "lb")) {
            if(process && process->is_running()) {
                auto breakpoints = process->get_breakpoints_with_index();
                if(breakpoints.empty()) {
                    std::cout << "No breakpoints set." << std::endl;
                } else {
                    std::cout << "Breakpoints:" << std::endl;
                    for(const auto& bp : breakpoints) {
                        std::cout << "  " << bp.first << ": " << format_hex64(bp.second) << std::dec << std::endl;
                    }
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && tokens[0] == "list_less") {
            try {
                std::ostringstream stream;
                stream << "objdump -d " << program_name << " | less ";
                std::string command = stream.str();
                if(system(command.c_str()) == 0) {
                    std::cout << "Succesful disassembly listing...\n";
                } else {
                    std::cout << "Failed disassembly listing...\n";
                }
            } catch(mx::Exception &e) {
                std::cerr << e.what() << "\n";
            }
            return true;
        }
        else if (tokens.size() == 3 && tokens[0] == "set") {
            if (process && process->is_running()) {
                try {
                    std::string reg_name = tokens[1];
                    uint64_t value = std::stoull(tokens[2], nullptr, 0);
                    
                    if (reg_name.length() == 3 && reg_name[0] == 'r') {
                        process->set_register(reg_name, value);
                        std::cout << "Register " << reg_name << " set to: " << format_hex64(value) << std::dec << std::endl;
                    } else if (reg_name.length() == 3 && reg_name[0] == 'e') {
                        process->set_register_32(reg_name, static_cast<uint32_t>(value));
                        std::cout << "Register " << reg_name << " set to: " << format_hex32(value) << std::dec << std::endl;
                    } else if (reg_name.length() == 2 && (reg_name[1] == 'l' || reg_name[1] == 'h')) {            
                        process->set_register_8(reg_name, static_cast<uint8_t>(value));
                        std::cout << "Register " << reg_name << " set to: " << format_hex8(value) << std::dec << std::endl;
                    } else if (reg_name.length() == 3 && reg_name[2] == 'b') {
                        process->set_register_8(reg_name, static_cast<uint8_t>(value));
                        std::cout << "Register " << reg_name << " set to: " << format_hex8(value) << std::dec << std::endl;
                    } else if (reg_name.length() == 2 || (reg_name.length() == 3 && reg_name[2] == 'w')) {
                        process->set_register_16(reg_name, static_cast<uint16_t>(value));
                        std::cout << "Register " << reg_name << " set to: " << format_hex16(value) << std::dec << std::endl;
                    } else {
                        process->set_register(reg_name, value);
                        std::cout << "Register " << reg_name << " set to: " << format_hex64(value) << std::dec << std::endl;
                    }
                    
                } catch (const std::exception& e) {
                    std::cerr << "Error setting register: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1  && (tokens[0] == "start" || tokens[0] == "restart")) {
            if (program_name.empty()) {
                std::cout << "No program to restart. Use launch command first or provide a program path." << std::endl;
                return true;
            }  
            std::cout << "Restarting program: " << program_name << std::endl;
            if (process) {
                try {
                    process->detach();
                    std::cout << "Detached from current process." << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Error detaching from process: " << e.what() << std::endl;
                }
                process.reset();
                p_id = -1;
            }
            try {
                std::filesystem::path exe_path(program_name);
                if (launch(exe_path, args_string)) {
                    std::cout << "Program restarted successfully." << std::endl;
                } else {
                    std::cout << "Failed to restart program." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error restarting program: " << e.what() << std::endl;
            }      
            return true;
        } else if (tokens.size() == 1 && tokens[0] == "pwd") {
            if (process && process->is_running()) {
                std::string cwd_link = "/proc/" + std::to_string(process->get_pid()) + "/cwd";
                char buf[PATH_MAX];
                ssize_t len = readlink(cwd_link.c_str(), buf, sizeof(buf)-1);
                if (len != -1) {
                    buf[len] = '\0';
                    std::cout << "Process CWD: " << buf << std::endl;
                } else {
                    std::cout << "Could not read process CWD." << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (tokens.size() == 1 && (tokens[0] == "info" || tokens[0] == "args")) {
            if (!args_string.empty()) {
                std::cout << "Program arguments: " << args_string << std::endl;
            } else {
                std::cout << "No arguments recorded." << std::endl;
            }
            return true;
        }    
        std::cout << "Unknown command: " << cmd << std::endl;
        return true;
    }

    void Debugger::step() {
        if (!process) {
            std::cerr << "No process attached or launched." << std::endl;
            return;
        }
        
        try {
            print_current_instruction();
            process->single_step();
            process->wait_for_single_step();
            std::cout << "Step completed." << std::endl;
            if(request) {
                try {
                    if(color_)
                        std::cout << Color::CYAN;
                    std::string response = request->generateTextWithCallback([](const std::string &chunk) {
                        std::cout << chunk << std::flush; 
                    });
                    if(color_)
                        std::cout << Color::RESET;
                    std::cout << "\n";
                } catch (const mx::ObjectRequestException &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                } 
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during single step: " << e.what() << std::endl;
        }
    }

    void Debugger::step_n(int count) {
        if (!process) {
            std::cerr << "No process attached or launched." << std::endl;
            return;
        }
        std::cout << "Stepping " << count << " instructions..." << std::endl;
        for (int i = 0; i < count; ++i) {
            try {
                print_current_instruction();
                process->single_step();
                process->wait_for_single_step();
            } catch (const std::exception& e) {
                std::cerr << "Error during step " << (i + 1) << ": " << e.what() << std::endl;
                break;
            }
        }
        std::cout << "Steps completed...\n";
    }

    void Debugger::print_current_instruction() {
        try {
            uint64_t rip = process->get_register("rip");
            std::vector<uint8_t> instruction_bytes = process->read_memory(rip, 15);

            if (process->has_breakpoint(rip)) {
                uint8_t original_byte = process->get_original_instruction(rip);
                instruction_bytes[0] = original_byte;  
                if(color_)
                    std::cout << Color::YELLOW;
                std::cout << "Current instruction at " << format_hex64(rip) << std::dec << " [BREAKPOINT]: ";
                if(color_)
                    std::cout << Color::RESET;
                code.str("");
                code << "New breakpoint at :" << format_hex64(rip) << "\n";
            } else {
                std::cout << "Current instruction at: " << format_hex64(rip) << std::dec << ": ";
            }

            std::string  random_name ;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 25);  
            for(size_t i = 0; i < 10; ++i) {
                random_name += 'a' + dis(gen);
            }
            std::string fullname = "/tmp/mxdbg_temp" + random_name + ".bin";
            class RemoveFileRaii {
            public:
                RemoveFileRaii(const std::string &name) : fullname(name) {}
                ~RemoveFileRaii() {
                    if(std::filesystem::exists(fullname))
                        std::filesystem::remove(fullname);
                }
            private:
                std::string fullname;
            };

            std::ofstream temp(fullname, std::ios::binary);
            temp.write(reinterpret_cast<const char*>(instruction_bytes.data()), instruction_bytes.size());
            temp.close();
            RemoveFileRaii remove_file(fullname);
            std::string cmd = "objdump -D -b binary -m i386:x86-64 " + fullname + " 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                std::cerr << "Failed to run objdump" << std::endl;
                return;
            }         
            char buffer[1024];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);
            std::istringstream ss(result);
            std::string line;
            std::ostringstream output;
            if(color_)
                std::cout << Color::RED;
            while (std::getline(ss, line)) {
                if (line.find("   0:") != std::string::npos) {
                    size_t colon_pos = line.find(":");
                    size_t tab_pos = line.find('\t', colon_pos);
                    
                    if (colon_pos != std::string::npos && tab_pos != std::string::npos) {
                        std::string hex_part = line.substr(colon_pos + 1, tab_pos - colon_pos - 1);
                        std::string instr_part = line.substr(tab_pos + 1);
                        
                        std::cout << hex_part << " -> " << instr_part << std::endl;
                        output << hex_part << " " << instr_part << std::endl;
                        code << output.str();
                    } else {
                        std::cout << line << std::endl;
                        output <<  line << std::endl;
                    }
                    break; 
                }
            }
            if(color_)
                std::cout << Color::RESET;
            if(request) {
                request->setPrompt("Explain this elf64-x86-64 instruction in one or two sentences for the user at the level of " + user_mode + ": " + output.str());
            }
            std::filesystem::remove(fullname);

        } catch (const std::exception& e) {
            std::cerr << "Error reading current instruction: " << e.what() << std::endl;
        }
    }

    uint64_t Debugger::get_base_address() const {
        if(process && process->is_running()) {
            std::fstream file;
            file.open(std::string("/proc/") + std::to_string(get_pid()) + std::string("/maps"), std::ios::in);
            if (!file.is_open()) {
                std::cerr << "Failed to open /proc/" << std::to_string(get_pid()) << "/maps" << std::endl;
                throw mx::Exception("Failed to open /proc/maps file.");
                return 0;
            }
            std::string line;
            std::string input;
            std::filesystem::path program_path = std::filesystem::absolute(program_name);
            while (std::getline(file, line)) {
                if (line.find("r-xp") != std::string::npos && line.find(program_path.filename().string()) != std::string::npos) {
                    auto pos = line.find_first_of('-');
                    if (pos != std::string::npos) {
                        std::cout << line << "\n";
                        std::string address_str = line.substr(0, pos);
                        uint64_t base_address = std::stoull(address_str, nullptr, 16);
                        file.close();
                        return base_address;
                    } else {
                        std::cerr << "Invalid address format in /proc/maps." << std::endl;
                        throw mx::Exception("Invalid address format in /proc/maps.");
                    }
                    file.close();
                    std::cout << "Base address not found." << std::endl;
                    return 0;
                }
            }
            file.close();
        }
        throw mx::Exception("Base address not found in /proc/maps.");
        return 0;
    }

    bool Debugger::is_valid_code_address(uint64_t address) const {
        std::fstream maps;
        std::string maps_path = "/proc/" + std::to_string(process->get_pid()) + "/maps";
        maps.open(maps_path, std::ios::in);        
        if (!maps.is_open()) {
            return address >= 0x400000 && address < 0x8000000000000000ULL; 
        }
        
        std::string line;
        while (std::getline(maps, line)) {
            if (line.find("r-xp") != std::string::npos || line.find("r-x") != std::string::npos) {
                size_t dash_pos = line.find('-');
                if (dash_pos != std::string::npos) {
                    std::string start_str = line.substr(0, dash_pos);
                    std::string end_str = line.substr(dash_pos + 1, line.find(' ') - dash_pos - 1);
                    
                    try {
                        uint64_t start_addr = std::stoull(start_str, nullptr, 16);
                        uint64_t end_addr = std::stoull(end_str, nullptr, 16);
                        
                        if (address >= start_addr && address < end_addr) {
                            return true;
                        }
                    } catch (...) {
                    }
                }
            }
        }
        return false;
    }

    void Debugger::print_address() const {     
        try {
            uint64_t base_address = get_base_address();
            if (base_address != 0) {
                std::cout << "Base address: " << format_hex64(base_address) << std::dec << std::endl;
            } else {
                std::cout << "Base address not found." << std::endl;
            }
        } catch (const mx::Exception& e) {
            std::cerr << "Error getting base address: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
        }
        if (process) {
            try {
                uint64_t pc = process->get_pc();
                std::cout << "Current PC: " << format_hex64(pc) << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error getting current PC: " << e.what() << std::endl;
            }
        } else {
            std::cout << "No process attached or running." << std::endl;
        }
    }
    
    std::string Debugger::functionText(const std::string &text) {
        std::string function_name = text; 
        std::string disassembly = obj_dump();
        std::ostringstream function_disassembly;
        bool in_function = false;
        std::istringstream iss(disassembly);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.find("<" + function_name + ">:") != std::string::npos) {
                in_function = true;
                function_disassembly << line << "\n";
                continue;
            }
            if (in_function) {
                if (line.find("Disassembly") != std::string::npos || line.find("<") != std::string::npos && line.find(">:") != std::string::npos) {
                    break;
                }
                if (!line.empty() && line.find_first_not_of(" \t\n\r") != std::string::npos) {
                    function_disassembly << line << "\n";
                }
            }
        }
        return function_disassembly.str();
    }

    void Debugger::print_backtrace() const {
        if (!process || !process->is_running()) {
            std::cout << "No process attached or running." << std::endl;
            return;
        }

        try {
            uint64_t rip = process->get_register("rip");
            uint64_t rbp = process->get_register("rbp");
            uint64_t rsp = process->get_register("rsp");
            
            std::cout << "Debug: RIP=" << format_hex64(rip) << ", RBP=" << format_hex64(rbp) 
                    << ", RSP=" << format_hex64(rsp) << std::endl;
            
            std::vector<uint64_t> frames = get_stack_frames();
            
            if(color_)
                std::cout << Color::BRIGHT_YELLOW;
            std::cout << "Backtrace:" << std::endl;
            if(color_)
                std::cout << Color::RESET;
                
            for (size_t i = 0; i < frames.size(); ++i) {
                std::string symbol = resolve_symbol(frames[i]);
                
                if(color_)
                    std::cout << Color::CYAN;
                std::cout << "#" << i << "  ";
                if(color_)
                    std::cout << Color::WHITE;
                std::cout << format_hex64(frames[i]) << std::dec;
                if(color_)
                    std::cout << Color::GREEN;
                std::cout << " in " << symbol << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error generating backtrace: " << e.what() << std::endl;
        }
    }

    std::vector<uint64_t> Debugger::get_stack_frames() const {
        std::vector<uint64_t> frames;
        
        try {
            uint64_t rbp = process->get_register("rbp");
            uint64_t rip = process->get_register("rip");
            uint64_t rsp = process->get_register("rsp");
            frames.push_back(rip);
            if (rbp == 0 || rbp < rsp || rbp > 0x7fffffffffff) {
                std::cout << "Warning: Invalid or uninitialized frame pointer (RBP=" 
                        << format_hex64(rbp) << ")" << std::endl;
                std::cout << "This usually means the function prologue hasn't executed yet." << std::endl;
                try {
                    std::vector<uint8_t> ret_data = process->read_memory(rsp, 8);
                    uint64_t return_address = 0;
                    std::memcpy(&return_address, ret_data.data(), 8);
                    if (return_address >= 0x400000 && return_address <= 0x500000) {
                        frames.push_back(return_address);
                        std::cout << "Found potential return address on stack: " 
                                << format_hex64(return_address) << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Could not read return address from stack: " << e.what() << std::endl;
                }
                
                return frames;
            }
            
            uint64_t current_rbp = rbp;
            const size_t max_frames = 20;
            
            for (size_t i = 0; i < max_frames && current_rbp != 0; ++i) {
                try {
                    if (current_rbp < rsp || current_rbp > 0x7fffffffffff) {
                        std::cout << "RBP validation failed: " << format_hex64(current_rbp) << std::endl;
                        break;
                    }
                    
                    std::vector<uint8_t> rbp_data = process->read_memory(current_rbp, 8);
                    uint64_t next_rbp = 0;
                    std::memcpy(&next_rbp, rbp_data.data(), 8);
                    
                    std::vector<uint8_t> ret_data = process->read_memory(current_rbp + 8, 8);
                    uint64_t return_address = 0;
                    std::memcpy(&return_address, ret_data.data(), 8);

                    if (!is_valid_code_address(return_address)) {
                        std::cout << "Return address not in executable memory: " << format_hex64(return_address) << std::endl;
                        break;
                    }
                    
                    if (next_rbp != 0 && (next_rbp <= current_rbp || next_rbp > 0x7fffffffffff)) {
                        std::cout << "Stack frame chain validation failed" << std::endl;
                        break;
                    }
                    
                    frames.push_back(return_address);
                    current_rbp = next_rbp;
                    
                } catch (const std::exception& e) {
                    std::cout << "Error reading stack frame " << i << ": " << e.what() << std::endl;
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error walking stack: " << e.what() << std::endl;
        }
        
        return frames;
    }

    std::string Debugger::resolve_symbol(uint64_t address) const {
        try {
            std::ostringstream addr2line_cmd;
            addr2line_cmd << "addr2line -f -C -e " << program_name << " 0x" << std::hex << address << " 2>/dev/null";
            
            FILE* pipe = popen(addr2line_cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                std::string result;
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    result = buffer;
                    if (!result.empty() && result.back() == '\n') {
                        result.pop_back();
                    }
                    fgets(buffer, sizeof(buffer), pipe);
                }
                pclose(pipe);
                
                if (!result.empty() && result != "??" && result.find("??") == std::string::npos) {
                    return result;
                }
            }
            
            std::ostringstream nm_cmd;
            nm_cmd << "nm -C " << program_name << " 2>/dev/null | grep -E '^[0-9a-fA-F]+ [tT]' | sort";
            
            pipe = popen(nm_cmd.str().c_str(), "r");
            if (!pipe) {
                return "<unknown>";
            }
            
            std::vector<std::pair<uint64_t, std::string>> symbols;
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string line(buffer);
                if (line.empty()) continue;
                
                std::istringstream iss(line);

                std::string addr_str, type, symbol_name;
                if (iss >> addr_str >> type >> symbol_name) {
                    try {
                        uint64_t symbol_addr = std::stoull(addr_str, nullptr, 16);
                        symbols.push_back({symbol_addr, symbol_name});
                    } catch (...) {
                        continue;
                    }
                }
            }
            pclose(pipe);
            
            std::string best_symbol = "<unknown>";
            uint64_t best_addr = 0;
            
            for (const auto& sym : symbols) {
                if (sym.first <= address && sym.first > best_addr) {
                    best_addr = sym.first;

                    best_symbol = sym.second;
                }
            }
            
            if (best_addr > 0 && best_symbol != "<unknown>") {
                uint64_t offset = address - best_addr;
                if (offset > 0) {
                    return best_symbol + "+" + std::to_string(offset);
                } else {
                    return best_symbol;
                }
            }
            return "<unknown>";
            
        } catch (const std::exception& e) {
            return "<unknown>";
        }
    }

    bool Debugger::is_at_function_entry() const {
        try {
            uint64_t rip = process->get_register("rip");
            std::vector<uint8_t> code = process->read_memory(rip, 8);
            if (code.size() >= 4 && 
                code[0] == 0xf3 && code[1] == 0x0f && code[2] == 0x1e && code[3] == 0xfa) {
                return true; 
            }
            if (code.size() >= 1 && code[0] == 0x55) {
                return true; 
            }
            return false;
        } catch (const std::exception& e) {
            return false;
        }
    }   

    uint64_t Debugger::calculate_variable_address(const std::string &reg_name, uint64_t offset) {
        try {
            uint64_t reg_value = process->get_register(reg_name);
            if (reg_name == "rbp") {
                return reg_value - offset;
            }
            else if (reg_name == "rsp") {
                return reg_value - offset;
            }
            else if (reg_name == "rip") {
                return reg_value + offset;
            }
            else {
                return reg_value + offset;
            }        
        } catch (const std::exception& e) {
            std::cerr << "Error getting register '" << reg_name << "': " << e.what() << std::endl;
            throw mx::Exception("Failed to calculate variable address for register: " + reg_name);
        }
        return 0;
    }

    void Debugger::print_memory_maps() const {
        std::ifstream maps("/proc/" + std::to_string(process->get_pid()) + "/maps");
        if(!maps.is_open()) 
            throw mx::Exception("Could not open memory map..");

        std::string line;
        
        std::cout << "Memory Map for PID " << process->get_pid() << ":" << std::endl;
        std::cout << "Address Range        Perms   Offset     Device   Inode    Pathname" << std::endl;
        std::cout << "=================================================================" << std::endl;
        
        while (std::getline(maps, line)) {
            
            if (line.find("r-xp") != std::string::npos) {
                if (color_) std::cout << Color::BRIGHT_GREEN;
            } else if (line.find("rw-p") != std::string::npos) {
                if (color_) std::cout << Color::BRIGHT_YELLOW;
            } else if (line.find("[heap]") != std::string::npos) {
                if (color_) std::cout << Color::BRIGHT_RED;
            } else if (line.find("[stack]") != std::string::npos) {
                if (color_) std::cout << Color::BRIGHT_BLUE;
            }
            
            std::cout << line << std::endl;
            
            if (color_) std::cout << Color::RESET;
        }
    }

    void Debugger::analyze_current_frame() const {
        try {
            uint64_t rbp = process->get_register("rbp");
            uint64_t rsp = process->get_register("rsp");
            uint64_t rip = process->get_register("rip");
            
            std::cout << "=== Current Stack Frame ===" << std::endl;
            std::cout << "RIP: " << format_hex64(rip) << " (" << resolve_symbol(rip) << ")" << std::endl;
            std::cout << "RBP: " << format_hex64(rbp) << std::endl;
            std::cout << "RSP: " << format_hex64(rsp) << std::endl;
            std::cout << "Frame Size: " << (rbp - rsp) << " bytes" << std::endl;
            
            std::cout << "\nStack Contents:" << std::endl;
            for (uint64_t addr = rsp; addr <= rbp + 8; addr += 8) {
                try {
                    auto data = process->read_memory(addr, 8);
                    uint64_t value = 0;
                    std::memcpy(&value, data.data(), 8);
                    
                    std::string annotation = "";
                    if (addr == rbp) annotation = " <- RBP";
                    else if (addr == rsp) annotation = " <- RSP";
                    else if (addr == rbp + 8) annotation = " <- Return Address";
                    
                    std::cout << format_hex64(addr) << ": " << format_hex64(value) 
                            << " (" << resolve_symbol(value) << ")" << annotation << std::endl;
                } catch (...) {
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error analyzing frame: " << e.what() << std::endl;
        }
    }

    std::vector<MemoryRegion> Debugger::get_searchable_memory_regions() {
        std::vector<MemoryRegion> regions;
        
        std::ifstream maps("/proc/" + std::to_string(process->get_pid()) + "/maps");
        if (!maps.is_open()) {
            throw mx::Exception("Could not open memory maps");
        }
        
        std::string line;
        while (std::getline(maps, line)) {
            std::istringstream iss(line);
            std::string addr_range, perms, offset, dev, inode, pathname;
            
            if (iss >> addr_range >> perms >> offset >> dev >> inode) {
                std::getline(iss, pathname); 
                if (perms[0] != 'r') continue;
                size_t dash_pos = addr_range.find('-');
                if (dash_pos != std::string::npos) {
                    std::string start_str = addr_range.substr(0, dash_pos);
                    std::string end_str = addr_range.substr(dash_pos + 1);
                    
                    try {
                        uint64_t start = std::stoull(start_str, nullptr, 16);
                        uint64_t end = std::stoull(end_str, nullptr, 16);
                        
                        if (end - start > 100 * 1024 * 1024) continue; 
                        
                        regions.push_back({start, end, perms, pathname});
                    } catch (...) {
                        continue;
                    }
                }
            }
        }
        
        return regions;
    }

    void Debugger::search_memory_for_int32(int32_t value) {
        auto regions = get_searchable_memory_regions();
        std::vector<uint8_t> search_bytes(4);
        std::memcpy(search_bytes.data(), &value, 4);
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Searching for int32 value: " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        if (color_) std::cout << Color::RESET;
        
        int found_count = 0;
        
        for (const auto& region : regions) {
            try {
                size_t region_size = region.end - region.start;
                if (region_size > 10 * 1024 * 1024) continue; 
                
                auto memory = process->read_memory(region.start, region_size);
                auto matches = find_in_memory(memory, search_bytes);
                
                for (size_t offset : matches) {
                    uint64_t address = region.start + offset;
                    
                    if (color_) std::cout << Color::BRIGHT_GREEN;
                    std::cout << "Found at: " << format_hex64(address);
                    if (color_) std::cout << Color::YELLOW;
                    std::cout << " [" << region.permissions << "] ";
                    if (color_) std::cout << Color::CYAN;
                    std::cout << region.pathname << std::endl;
                    if (color_) std::cout << Color::RESET;
                    
                    found_count++;
                    if (found_count >= 20) {
                        std::cout << "... (limiting output to first 20 matches)" << std::endl;
                        return;
                    }
                }
            } catch (const std::exception& e) {
                continue;
            }
        }
        
        if (found_count == 0) {
            std::cout << "Value not found in readable memory." << std::endl;
        } else {
            std::cout << "Total matches found: " << found_count << std::endl;
        }
    }

    void Debugger::search_memory_for_int64(int64_t value) {
        auto regions = get_searchable_memory_regions();
        std::vector<uint8_t> search_bytes(8);
        std::memcpy(search_bytes.data(), &value, 8);
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Searching for int64 value: " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        if (color_) std::cout << Color::RESET;
        
        int found_count = 0;
        
        for (const auto& region : regions) {
            try {
                size_t region_size = region.end - region.start;
                if (region_size > 10 * 1024 * 1024) continue;
                
                auto memory = process->read_memory(region.start, region_size);
                auto matches = find_in_memory(memory, search_bytes);
                
                for (size_t offset : matches) {
                    uint64_t address = region.start + offset;
                    
                    if (color_) std::cout << Color::BRIGHT_GREEN;
                    std::cout << "Found at: " << format_hex64(address);
                    if (color_) std::cout << Color::YELLOW;
                    std::cout << " [" << region.permissions << "] ";
                    if (color_) std::cout << Color::CYAN;
                    std::cout << region.pathname << std::endl;
                    if (color_) std::cout << Color::RESET;
                    
                    found_count++;
                    if (found_count >= 20) {
                        std::cout << "... (limiting output to first 20 matches)" << std::endl;
                        return;
                    }
                }
            } catch (const std::exception& e) {
                continue;
            }
        }
        
        if (found_count == 0) {
            std::cout << "Value not found in readable memory." << std::endl;
        } else {
            std::cout << "Total matches found: " << found_count << std::endl;
        }
    }

    void Debugger::search_memory_for_string(const std::string& pattern) {
        auto regions = get_searchable_memory_regions();
        std::vector<uint8_t> search_bytes(pattern.begin(), pattern.end());
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Searching for string: \"" << pattern << "\"" << std::endl;
        if (color_) std::cout << Color::RESET;
        
        int found_count = 0;
        
        for (const auto& region : regions) {
            try {
                size_t region_size = region.end - region.start;
                if (region_size > 10 * 1024 * 1024) continue;
                
                auto memory = process->read_memory(region.start, region_size);
                auto matches = find_in_memory(memory, search_bytes);
                
                for (size_t offset : matches) {
                    uint64_t address = region.start + offset;
                    
                    std::string context;
                    try {
                        size_t context_start = (offset > 20) ? offset - 20 : 0;
                        size_t context_end = std::min(offset + pattern.length() + 20, memory.size());
                        
                        for (size_t i = context_start; i < context_end; ++i) {
                            char c = static_cast<char>(memory[i]);
                            if (std::isprint(c)) {
                                context += c;
                            } else {
                                context += '.';
                            }
                        }
                    } catch (...) {
                        context = "(context unavailable)";
                    }
                    
                    if (color_) std::cout << Color::BRIGHT_GREEN;
                    std::cout << "Found at: " << format_hex64(address);
                    if (color_) std::cout << Color::YELLOW;
                    std::cout << " [" << region.permissions << "] ";
                    if (color_) std::cout << Color::WHITE;
                    std::cout << "Context: \"" << context << "\"";
                    if (color_) std::cout << Color::CYAN;
                    std::cout << " " << region.pathname << std::endl;
                    if (color_) std::cout << Color::RESET;
                    
                    found_count++;
                    if (found_count >= 10) {
                        std::cout << "... (limiting output to first 10 matches)" << std::endl;
                        return;
                    }
                }
            } catch (const std::exception& e) {
                continue;
            }
        }
        
        if (found_count == 0) {
            std::cout << "String not found in readable memory." << std::endl;
        } else {
            std::cout << "Total matches found: " << found_count << std::endl;
        }
    }

    void Debugger::search_memory_for_bytes(const std::vector<std::string>& byte_tokens) {
        std::vector<uint8_t> search_bytes;
        
        for (const std::string& token : byte_tokens) {
            try {
                uint8_t byte = static_cast<uint8_t>(std::stoull(token, nullptr, 16));
                search_bytes.push_back(byte);
            } catch (...) {
                std::cout << "Invalid hex byte: " << token << std::endl;
                return;
            }
        }
        
        if (search_bytes.empty()) {
            std::cout << "No valid bytes to search for." << std::endl;
            return;
        }
        
        auto regions = get_searchable_memory_regions();
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Searching for byte pattern: ";
        for (uint8_t b : search_bytes) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
        }
        std::cout << std::dec << std::endl;
        if (color_) std::cout << Color::RESET;
        
        int found_count = 0;
        
        for (const auto& region : regions) {
            try {
                size_t region_size = region.end - region.start;
                if (region_size > 10 * 1024 * 1024) continue;
                
                auto memory = process->read_memory(region.start, region_size);
                auto matches = find_in_memory(memory, search_bytes);
                
                for (size_t offset : matches) {
                    uint64_t address = region.start + offset;
                    
                    if (color_) std::cout << Color::BRIGHT_GREEN;
                    std::cout << "Found at: " << format_hex64(address);
                    if (color_) std::cout << Color::YELLOW;
                    std::cout << " [" << region.permissions << "] ";
                    if (color_) std::cout << Color::CYAN;
                    std::cout << region.pathname << std::endl;
                    if (color_) std::cout << Color::RESET;
                    
                    found_count++;
                    if (found_count >= 20) {
                        std::cout << "... (limiting output to first 20 matches)" << std::endl;
                        return;
                    }
                }
            } catch (const std::exception& e) {
                continue;
            }
        }
        
        if (found_count == 0) {
            std::cout << "Byte pattern not found in readable memory." << std::endl;
        } else {
            std::cout << "Total matches found: " << found_count << std::endl;
        }
    }

    void Debugger::search_memory_for_pattern(const std::string& pattern) {
        std::vector<std::pair<uint8_t, bool>> parsed_pattern; 
        std::string clean_pattern;
        for (char c : pattern) {
            if (c != ' ' && c != '\t') {
                clean_pattern += std::toupper(c);
            }
        }
        
        
        if (!parse_pattern(clean_pattern, parsed_pattern)) {
            std::cout << "Invalid pattern format. Use hex bytes with ?? for wildcards." << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  41 ?? 43    (hex bytes with wildcard)" << std::endl;
            std::cout << "  A?B         (short hex notation)" << std::endl;
            std::cout << "  48 8B ??    (mix of hex and wildcards)" << std::endl;
            return;
        }
        
        if (parsed_pattern.empty()) {
            std::cout << "Empty pattern." << std::endl;
            return;
        }
        
        auto regions = get_searchable_memory_regions();
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Searching for pattern: ";
        for (const auto& p : parsed_pattern) {
            if (p.second) {
                std::cout << "?? ";
            } else {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)p.first << " ";
            }
        }
        std::cout << std::dec << std::endl;
        if (color_) std::cout << Color::RESET;
        
        int found_count = 0;
        
        for (const auto& region : regions) {
            try {
                size_t region_size = region.end - region.start;
                if (region_size > 10 * 1024 * 1024) continue;
                
                auto memory = process->read_memory(region.start, region_size);
                auto matches = find_pattern_in_memory(memory, parsed_pattern);
                
                for (size_t offset : matches) {
                    uint64_t address = region.start + offset;
                    
                    std::string hex_context;
                    size_t context_start = (offset > 8) ? offset - 8 : 0;
                    size_t context_end = std::min(offset + parsed_pattern.size() + 8, memory.size());
                    
                    for (size_t i = context_start; i < context_end; ++i) {
                        if (i == offset) hex_context += "[";
                        hex_context += format_hex8(memory[i]);
                        if (i == offset + parsed_pattern.size() - 1) hex_context += "]";
                        hex_context += " ";
                    }
                    
                    if (color_) std::cout << Color::BRIGHT_GREEN;
                    std::cout << "Found at: " << format_hex64(address);
                    if (color_) std::cout << Color::YELLOW;
                    std::cout << " [" << region.permissions << "] ";
                    if (color_) std::cout << Color::WHITE;
                    std::cout << "Context: " << hex_context;
                    if (color_) std::cout << Color::CYAN;
                    std::cout << region.pathname << std::endl;
                    if (color_) std::cout << Color::RESET;
                    
                    found_count++;
                    if (found_count >= 20) {
                        std::cout << "... (limiting output to first 20 matches)" << std::endl;
                        return;
                    }
                }
            } catch (const std::exception& e) {
                continue; 
            }
        }
        
        if (found_count == 0) {
            std::cout << "Pattern not found in readable memory." << std::endl;
        } else {
            std::cout << "Total matches found: " << found_count << std::endl;
        }
    }

    bool Debugger::parse_pattern(const std::string& pattern, std::vector<std::pair<uint8_t, bool>>& parsed_pattern) {
        parsed_pattern.clear();
        if (pattern.find(' ') != std::string::npos) {
            std::istringstream iss(pattern);
            std::string token;
            while (iss >> token) {
                if (token == "??" || token == "?") {
                    parsed_pattern.push_back({0, true}); 
                } else {
                    try {
                        if (token.length() == 2) {
                            uint8_t byte = static_cast<uint8_t>(std::stoull(token, nullptr, 16));
                            parsed_pattern.push_back({byte, false});
                        } else {
                            return false; 
                        }
                    } catch (...) {
                        return false;
                    }
                }
            }
            return !parsed_pattern.empty();
        }

        for (size_t i = 0; i < pattern.length(); ) {
            if (i + 1 < pattern.length() && pattern[i] == '?' && pattern[i + 1] == '?') {
                parsed_pattern.push_back({0, true}); 
                i += 2;
            } else if (pattern[i] == '?') {
                parsed_pattern.push_back({0, true});
                i += 1;
            } else if (i + 1 < pattern.length() && std::isxdigit(pattern[i]) && std::isxdigit(pattern[i + 1])) {
                try {
                    std::string hex_str = pattern.substr(i, 2);
                    uint8_t byte = static_cast<uint8_t>(std::stoull(hex_str, nullptr, 16));
                    parsed_pattern.push_back({byte, false});
                    i += 2;
                } catch (...) {
                    return false;
                }
            } else {
                return false; 
            }
        }
        return !parsed_pattern.empty();
    }

    std::vector<size_t> Debugger::find_pattern_in_memory(const std::vector<uint8_t>& memory,                                                         const std::vector<std::pair<uint8_t, bool>>& pattern) {
        std::vector<size_t> matches;
        if (pattern.empty() || memory.size() < pattern.size()) {
            return matches;
        }
        
        for (size_t i = 0; i <= memory.size() - pattern.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (!pattern[j].second) { 
                    if (memory[i + j] != pattern[j].first) {
                        found = false;
                        break;
                    }
                }
            }
            if (found) {
                matches.push_back(i);
            }
        }
       return matches;  
    }

    void Debugger::list_threads() {
        if (!process || !process->is_running()) {
            std::cout << "No process running." << std::endl;
            return;
        }
        
        std::string task_dir = "/proc/" + std::to_string(process->get_pid()) + "/task";
        
        if (color_) std::cout << Color::BRIGHT_CYAN;
        std::cout << "Threads for PID " << process->get_pid() << ":" << std::endl;
        std::cout << "  TID    State   RIP              Name" << std::endl;
        std::cout << "================================================" << std::endl;
        if (color_) std::cout << Color::RESET;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(task_dir)) {
                if (entry.is_directory()) {
                    std::string tid_str = entry.path().filename().string();
                    pid_t tid = std::stoi(tid_str);
                    
                    
                    std::ifstream stat_file(entry.path() / "stat");
                    std::string stat_line;
                    if (std::getline(stat_file, stat_line)) {
                        std::istringstream iss(stat_line);
                        std::string dummy, state;
                        iss >> dummy >> dummy >> state; 
                        
                        uint64_t rip = 0;
                        try {
                            struct user_regs_struct regs;
                            if (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == 0) {
                                rip = regs.rip;
                            }
                        } catch (...) {}
                        
                        std::string name = "thread-" + tid_str;
                        std::ifstream comm_file(entry.path() / "comm");
                        std::getline(comm_file, name);
                        
                        if (color_) std::cout << Color::WHITE;
                        std::cout << std::setw(6) << tid << "  " 
                                << std::setw(6) << state << "  "
                                << format_hex64(rip) << "  " << name << std::endl;
                        if (color_) std::cout << Color::RESET;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error listing threads: " << e.what() << std::endl;
        }
    }

    void Debugger::switch_thread(pid_t tid) {
        if (!process || !process->is_running()) {
            std::cout << "No process running." << std::endl;
            return;
        }
        std::string task_path = "/proc/" + std::to_string(process->get_pid()) + "/task/" + std::to_string(tid);
        if (!std::filesystem::exists(task_path)) {
            std::cout << "Thread " << tid << " does not exist." << std::endl;
            return;
        }
        try {
            process->switch_to_thread(tid);  
            std::cout << "Switched to thread " << tid << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error switching to thread " << tid << ": " << e.what() << std::endl;
        }
    }

    std::vector<size_t> Debugger::find_in_memory(const std::vector<uint8_t>& haystack, const std::vector<uint8_t>& needle) {
        std::vector<size_t> matches;        
        if (needle.empty() || haystack.size() < needle.size()) {
            return matches;
        }
        for (size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < needle.size(); ++j) {
                if (haystack[i + j] != needle[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                matches.push_back(i);
            }
        }
        return matches;
    }

    void Debugger::step_over() {
        if (!process || !process->is_running()) {
            std::cerr << "No process attached or running." << std::endl;
            return;
        }
        
        try {
            uint64_t current_rip = process->get_register("rip");
            std::vector<uint8_t> instruction = process->read_memory(current_rip, 15);
            bool is_call = false;
            if (instruction.size() >= 1) {
                if (instruction[0] == 0xE8 ||  
                    (instruction[0] == 0xFF && instruction.size() >= 2 && 
                    ((instruction[1] & 0x38) == 0x10)) ||  
                    instruction[0] == 0x9A) {  
                    is_call = true;
                }
            }
            
            if (is_call) {
                uint64_t return_address = current_rip;
                if (instruction[0] == 0xE8 && instruction.size() >= 5) {
                    return_address += 5; 
                } else if (instruction[0] == 0xFF) {
                    return_address += 2;
                } else {
                    return_address += 1;  
                }
                process->set_breakpoint(return_address);
                if (color_) std::cout << Color::BRIGHT_YELLOW;
                std::cout << "Step over: Setting temporary breakpoint at " 
                        << format_hex64(return_address) << std::endl;
                if (color_) std::cout << Color::RESET;
                process->continue_execution();
                process->wait_for_stop();
                process->remove_breakpoint(return_address);
                std::cout << "Step over completed." << std::endl;
            } else {
                print_current_instruction();
                process->single_step();
                process->wait_for_single_step();
                std::cout << "Step completed." << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error during step over: " << e.what() << std::endl;
        }
    }

   void Debugger::step_out() {
        if (!process || !process->is_running()) {
            std::cerr << "No process attached or running." << std::endl;
            return;
        }        
        
        try {
            uint64_t rip = process->get_register("rip");
            uint64_t rbp = process->get_register("rbp");
            uint64_t rsp = process->get_register("rsp");
            
            if (process->has_breakpoint(rip)) {
                std::cout << "Currently at breakpoint. Stepping over it first..." << std::endl;
                uint8_t original_byte = process->get_original_instruction(rip);
                long data = ptrace(PTRACE_PEEKDATA, process->get_current_thread(), rip, nullptr);
                long restored = (data & ~0xFF) | original_byte;
                ptrace(PTRACE_POKEDATA, process->get_current_thread(), rip, restored);
                ptrace(PTRACE_SINGLESTEP, process->get_current_thread(), nullptr, nullptr);
                int status;
                waitpid(process->get_current_thread(), &status, 0);
                
                data = ptrace(PTRACE_PEEKDATA, process->get_current_thread(), rip, nullptr);
                long data_with_int3 = (data & ~0xFF) | 0xCC;
                ptrace(PTRACE_POKEDATA, process->get_current_thread(), rip, data_with_int3);
                
                rip = process->get_register("rip");
            }
            
            if (rip < 0x400000 || rip > 0x500000) {
                std::cout << "Cannot step out: Process appears to be at entry point or invalid location." << std::endl;
                std::cout << "Try running the program first with 'run' or 'continue'." << std::endl;
                return;
            }
            uint64_t return_address = 0;
            if (rbp > 0 && rbp > rsp && rbp < 0x7fffffffffff && (rbp - rsp) <= 0x10000) {
                try {
                    std::vector<uint8_t> return_data = process->read_memory(rbp + 8, 8);
                    std::memcpy(&return_address, return_data.data(), 8);
                    
                    if (return_address >= 0x400000 && return_address < 0x500000 && 
                        is_valid_code_address(return_address)) {
                        std::cout << "Using return address from RBP+8: " << format_hex64(return_address) << std::endl;
                    }
                } catch (...) {
                    return_address = 0;
                }
            }
        
            if (return_address == 0 || return_address < 0x400000 || return_address >= 0x500000) {
                std::cout << "Searching for valid return address on stack..." << std::endl;
                
                for (int i = 0; i < 8; i++) {
                    try {
                        uint64_t stack_addr = rsp + (i * 8);
                        std::vector<uint8_t> stack_data = process->read_memory(stack_addr, 8);
                        uint64_t potential_return = 0;
                        std::memcpy(&potential_return, stack_data.data(), 8);
                        
                        if (potential_return >= 0x400000 && potential_return < 0x500000 && 
                            potential_return != rip && is_valid_code_address(potential_return)) {
                            
                            return_address = potential_return;
                            std::cout << "Using return address from RSP+" << (i * 8) 
                                    << ": " << format_hex64(return_address) << std::endl;
                            break;
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
            
            if (return_address == 0 || return_address < 0x400000 || return_address >= 0x500000) {
                std::cout << "Could not find valid return address. Try 'next' or 'until' instead." << std::endl;
                return;
            }
            process->set_breakpoint(return_address);   
            if (color_) std::cout << Color::BRIGHT_YELLOW;
            std::cout << "Step out: Setting breakpoint at return address " 
                    << format_hex64(return_address) << std::endl;
            if (color_) std::cout << Color::RESET;
            process->continue_execution();
            process->wait_for_stop();
            process->remove_breakpoint(return_address);       
            std::cout << "Step out completed." << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error during step out: " << e.what() << std::endl;
        }
    }
    void Debugger::run_until(uint64_t address) {
        if (!process || !process->is_running()) {
            std::cerr << "No process attached or running." << std::endl;
            return;
        }
        try {
            if (!is_valid_code_address(address)) {
                std::cerr << "Warning: Address " << format_hex64(address) 
                        << " may not be in executable memory." << std::endl;
            }
            uint64_t current_rip = process->get_register("rip");
            if (current_rip == address) {
                std::cout << "Already at target address " << format_hex64(address) << std::endl;
                return;
            }
            bool had_existing_breakpoint = process->has_breakpoint(address);        
            if (!had_existing_breakpoint) {
                process->set_breakpoint(address);
            }
            
            if (color_) std::cout << Color::BRIGHT_YELLOW;
            std::cout << "Running until address " << format_hex64(address);
            if (!had_existing_breakpoint) {
                std::cout << " (temporary breakpoint set)";
            }
            std::cout << "..." << std::endl;
            if (color_) std::cout << Color::RESET;
            process->continue_execution();
            process->wait_for_stop();
            uint64_t final_rip = process->get_register("rip");
            if (final_rip == address) {
                if (color_) std::cout << Color::BRIGHT_GREEN;
                std::cout << "Reached target address " << format_hex64(address) << std::endl;
                if (color_) std::cout << Color::RESET;
            } else {
                if (color_) std::cout << Color::BRIGHT_RED;
                std::cout << "Stopped at " << format_hex64(final_rip) 
                        << " (did not reach target " << format_hex64(address) << ")" << std::endl;
                if (color_) std::cout << Color::RESET;
            }
            if (!had_existing_breakpoint) {
                process->remove_breakpoint(address);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error during run until: " << e.what() << std::endl;
           try {
                if (!process->has_breakpoint(address)) {
                    process->remove_breakpoint(address);
                }
            } catch (...) {
                
            }
        }
    }
}