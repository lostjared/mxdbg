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
#include<termios.h>

namespace mx {
    
    namespace Color {
        const std::string RESET = "\033[0m";
        const std::string BOLD = "\033[1m";
        const std::string DIM = "\033[2m";
        const std::string BLACK = "\033[30m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string WHITE = "\033[37m";
        const std::string BRIGHT_RED = "\033[91m";
        const std::string BRIGHT_GREEN = "\033[92m";
        const std::string BRIGHT_YELLOW = "\033[93m";
        const std::string BRIGHT_BLUE = "\033[94m";
        const std::string BRIGHT_MAGENTA = "\033[95m";
        const std::string BRIGHT_CYAN = "\033[96m";        
        const std::string BG_RED = "\033[41m";
        const std::string BG_GREEN = "\033[42m";
        const std::string BG_YELLOW = "\033[43m";
    }   
    
    bool terminal_supports_color() {
        return isatty(STDOUT_FILENO) && getenv("TERM") != nullptr;
    }
    
    bool color_ = terminal_supports_color();

    std::vector<std::string> split_command(const std::string &cmd) {
        std::vector<std::string> tokenz;
        std::string token;
        std::istringstream tokens(cmd);
        while (tokens >> token) {
            tokenz.push_back(token);
        }
        return tokenz;
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

    Debugger::Debugger() : p_id(-1) {
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
            std::cout << "Process launched and stopped at entry point." << std::endl;
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

    bool Debugger::setfunction_breakpoint(const std::string &filename) {
        if (!process || !process->is_running()) {
            std::cout << "No process attached or running." << std::endl;
            return false;
        }
        
        std::string function_name = filename;
        if (function_name.empty()) {
            std::cout << "Usage: function <function_name>" << std::endl;
            return false;
        }
        
        try {
            std::string disassembly = obj_dump();
            std::istringstream iss(disassembly);
            std::string line;
            uint64_t function_offset = 0;   
            
            while (std::getline(iss, line)) {
                if (line.find("<" + function_name + ">:") != std::string::npos) {
                    size_t space_pos = line.find(' ');
                    if (space_pos != std::string::npos) {
                        std::string addr_str = line.substr(0, space_pos);
                        function_offset = std::stoull(addr_str, nullptr, 16);
                        break;
                    }
                }
            }
            
            if (function_offset == 0) {
                std::cout << "Function '" << function_name << "' not found in disassembly." << std::endl;
                return false;
            }
            
            bool is_pie = false;
            uint64_t base_address = 0;
            uint64_t file_offset = 0;
            uint64_t runtime_address = function_offset; 
            
            std::fstream file;
            file.open(std::string("/proc/") + std::to_string(get_pid()) + std::string("/maps"), std::ios::in);
            if (!file.is_open()) {
                std::cerr << "Failed to open /proc/maps" << std::endl;
                return false;
            }
            
            std::filesystem::path program_path = std::filesystem::absolute(program_name);
            std::string maps_line;
            while (std::getline(file, maps_line)) {
                if (maps_line.find("r-xp") != std::string::npos && 
                    maps_line.find(program_path.filename().string()) != std::string::npos) {
                    
                    std::istringstream iss(maps_line);
                    std::string addr_range, permissions, offset_str;
                    iss >> addr_range >> permissions >> offset_str;
                    
                    auto dash_pos = addr_range.find('-');
                    if (dash_pos != std::string::npos) {
                        std::string base_addr_str = addr_range.substr(0, dash_pos);
                        base_address = std::stoull(base_addr_str, nullptr, 16);
                    }
                    
                    file_offset = std::stoull(offset_str, nullptr, 16);
                    
                    if (base_address < 0x400000 || base_address > 0x500000) {
                        is_pie = true;
                    } else {
                        is_pie = false;
                    }
                    break;
                }
            }
            file.close();
            
            if (base_address == 0) {
                std::cout << "Could not find base address in /proc/maps" << std::endl;
                return false;
            }
            
            if (is_pie) {
                runtime_address = base_address + (function_offset - file_offset);
                
                if(color_)
                    std::cout << Color::BRIGHT_BLUE;
                std::cout << "Detected PIE executable" << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            } else {
                runtime_address = function_offset;
                
                if(color_)
                    std::cout << Color::BRIGHT_BLUE;
                std::cout << "Detected non-PIE executable" << std::endl;
                if(color_)
                    std::cout << Color::RESET;
            }
            
            process->set_breakpoint(runtime_address);
            
            if(color_)
                std::cout << Color::BRIGHT_GREEN;
            std::cout << "Breakpoint set at function '" << function_name << "'" << std::endl;
            std::cout << "  Function offset: " << format_hex64(function_offset) << std::endl;
            if (is_pie) {
                std::cout << "  Base address: " << format_hex64(base_address) << std::endl;
                std::cout << "  File offset: " << format_hex64(file_offset) << std::endl;
            }
            std::cout << "  Runtime address: " << format_hex64(runtime_address) << std::dec << std::endl;
            std::cout << "  PIE executable: " << (is_pie ? "Yes" : "No") << std::endl;
            if(color_)
                std::cout << Color::RESET;
            
        } catch (const std::exception& e) {
            std::cerr << "Error setting breakpoint: " << e.what() << std::endl;
        }
        
        return true;
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

    void expression(const std::string &text) {
        try {
            expr_parser::ExprLexer lexer(text);
            expr_parser::ExprParser parser(lexer);
            uint64_t result = parser.parse();
            std::cout << "The result: ";
            if(color_)
                std::cout << Color::BOLD;
            std::cout << format_hex64(result) << " | " << std::dec << result << "\n";
            if(color_)
                std::cout << Color::RESET;
        } catch (const std::exception& e) {
            std::cerr << "Error on expression: " << e.what() << "\n";
        }
    }
    
    bool Debugger::command(const std::string &cmd) {
        std::vector<std::string> tokens = split_command(cmd);
        if (tokens.empty()) {
            std::cout << "No command entered." << std::endl;
            return true;
        }
        if(tokens.size() >= 2 && tokens[0] == "expr") {
            std::string e = cmd.substr(cmd.find(' ') + 1);
            expression(e);
            return true;
        } else if(cmd == "run" || cmd == "r") {
            if(process  && process->is_running()) {
                try {
                    if(!setfunction_breakpoint("main")) {
                        std::cerr << "Could not set breakpoint..\n";
                        return true;
                    }
                    process->continue_execution();
                    process->wait_for_stop();
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
                } catch(const mx::Exception &e) {
                    std::cerr << "Error running process: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;

        } else if (cmd == "continue" || cmd == "c") {
            if (process && process->is_running()) {
                try {
                    uint64_t pc_before = process->get_pc(); 
                    if (process->has_breakpoint(pc_before)) {
                        uint8_t original_byte = process->get_original_instruction(pc_before);
                        long data = ptrace(PTRACE_PEEKDATA, process->get_pid(), pc_before, nullptr);
                        long restored = (data & ~0xFF) | original_byte;
                        ptrace(PTRACE_POKEDATA, process->get_pid(), pc_before, restored);
                        
                        
                        process->single_step();
                        process->wait_for_single_step();
                        
                        
                        data = ptrace(PTRACE_PEEKDATA, process->get_pid(), pc_before, nullptr);
                        long data_with_int3 = (data & ~0xFF) | 0xCC;
                        ptrace(PTRACE_POKEDATA, process->get_pid(), pc_before, data_with_int3);
                        
                        
                        process->continue_execution();
                        process->wait_for_stop();
                    } else {
                        process->continue_execution();
                        process->wait_for_stop();
                    }
                    if(process && process->is_running()) {    
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
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error during continue: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process running." << std::endl;
            }
            return true;
        } else if (cmd == "step" || cmd == "s") {
            step();
            return true;
        } else if (cmd.substr(0, 5) == "step " || cmd.substr(0, 2) == "s ") {
            try {
                std::string num_str = cmd.substr(cmd.find(' ') + 1);
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
        } else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
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
        } else if(cmd == "base") {
            if (process && process->is_running()) {
                print_address();
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;
        } else if(cmd == "list") {
            try {
                std::cout << obj_dump() << std::endl;
            } catch(mx::Exception &e) {
                std::cerr << "Error running objdump: " << e.what() << std::endl;
            }
            return true;
        }
        else if(cmd.length() >= 7 && cmd.substr(0, 7) == "explain" && cmd.length() > 7 && cmd[7] == ' ') {
            std::string function_name = cmd.substr(8); 
            if (function_name.empty()) {
                std::cout << "Usage: explain <function_name>" << std::endl;
                return true;
            }
            
            std::string disassembly;
            try {
                disassembly = obj_dump();
            } catch(mx::Exception &e) {
                std::cerr << "Error running objdump: " << e.what() << std::endl;
                return true;
            }
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
                    if (line.find("<") != std::string::npos && line.find(">:") != std::string::npos) {
                        break;
                    }
                    if (!line.empty() && line.find_first_not_of(" \t\n\r") != std::string::npos) {
                        function_disassembly << line << "\n";
                    }
                }
            }
            
            std::string function_code = function_disassembly.str();
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
        else if(cmd == "explain") {
            std::cout << "Usage: explain <function_name>" << std::endl;
            std::cout << "Example: explain main" << std::endl;
            std::cout << "To see available functions: objdump -t " << program_name << std::endl;
            return true;
        }
        else if (cmd == "status" || cmd == "st") {
            if (process) { 
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
        else if(tokens.size() == 2 && (tokens[0] == "search" || tokens[0] == "find")) {
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
        }
        else if (cmd == "help" || cmd == "h") {
            if(color_)
                std::cout << Color::YELLOW;
            std::cout << "Available commands:" << std::endl;
            std::cout << "  continue, c            - Continue process execution" << std::endl;
            std::cout << "  step, s                - Execute single instruction" << std::endl;
            std::cout << "  step N, s N            - Execute N instructions" << std::endl;
            std::cout << "  status, st             - Show process status" << std::endl;
            std::cout << "  list, list_less        - display disassembly " << std::endl;
            std::cout << "  registers, regs        - Show all registers" << std::endl;
            std::cout << "  register <name>        - Show specific register value" << std::endl;
            std::cout << "  set <reg> <value>      - Set register to value" << std::endl;
            std::cout << "  break <addr>, b        - Set breakpoint at address" << std::endl;
            std::cout << "  read <addr>            - Read memory at address" << std::endl;
            std::cout << "  write <addr> <value>   - Write value to memory at address" << std::endl;
            std::cout << "  write_bytes <ad> <va>  - Write bytes to address" << std::endl;
            std::cout << "  explain <function>     - Explain function disassembly with AI" << std::endl;
            std::cout << "  ask <question>         - Ask the AI a question about the prgoram" << std::endl;
            std::cout << "  user mode              - User Difficulty level for use with AI" << std::endl;
            std::cout << "  help, h                - Show this help message" << std::endl;
            std::cout << "  quit, q, exit          - Exit debugger" << std::endl;
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
                        size_t index = std::stoull(input);
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
        } else if (cmd == "start" || cmd == "restart") {
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
            process->single_step();
            process->wait_for_single_step();
            std::cout << "Step completed." << std::endl;
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
                process->single_step();
                process->wait_for_single_step();
                std::cout << "Step " << (i + 1) << " completed." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error during step " << (i + 1) << ": " << e.what() << std::endl;
                break;
            }
        }
    }

    void Debugger::print_current_instruction() {
        try {
            uint64_t rip = process->get_register("rip");
            std::vector<uint8_t> instruction_bytes = process->read_memory(rip, 15);

            if (process->has_breakpoint(rip)) {
                uint8_t original_byte = process->get_original_instruction(rip);
                instruction_bytes[0] = original_byte;  // Replace CC (int3) with original
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

    void Debugger::disasm_bytes(std::vector<uint8_t> &bytes) {
        std::ofstream temp("/tmp/mxdbg_temp.bin", std::ios::binary);
        temp.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        temp.close();
        std::string cmd = "objdump -D -b binary -m i386:x86-64 /tmp/mxdbg_temp.bin";
        std::cout << "Running command: " << cmd << std::endl;
        int result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Failed to execute objdump command." << std::endl;
        } else {
            std::cout << "Disassembly completed." << std::endl;
        }
        std::filesystem::remove("/tmp/mxdbg_temp.bin");
    }

    std::size_t Debugger::get_instruction_length(const std::vector<uint8_t>& bytes, size_t offset) {
        if (offset >= bytes.size()) return 1;
        
        try {
            std::ofstream temp("/tmp/mxdbg_instr.bin", std::ios::binary);
            temp.write(reinterpret_cast<const char*>(bytes.data() + offset), bytes.size() - offset);
            temp.close();
            
            std::string cmd = "objdump -D -b binary -m i386:x86-64 /tmp/mxdbg_instr.bin 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) return 1;
            
            char buffer[256];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);
            
            std::istringstream ss(result);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.find("   0:") != std::string::npos) {
                    size_t colon_pos = line.find(":");
                    if (colon_pos == std::string::npos) continue;
                    
                    size_t tab_pos = line.find('\t', colon_pos);
                    if (tab_pos == std::string::npos) continue;
                    
                    std::string hex_part = line.substr(colon_pos + 1, tab_pos - colon_pos - 1);
                    
                    size_t byte_count = 0;
                    std::istringstream hex_stream(hex_part);
                    std::string hex_byte;
                    while (hex_stream >> hex_byte) {
                        if (hex_byte.length() == 2 && std::isxdigit(hex_byte[0]) && std::isxdigit(hex_byte[1])) {
                            byte_count++;
                        }
                    }
                    
                    std::filesystem::remove("/tmp/mxdbg_instr.bin");
                    return byte_count > 0 ? byte_count : 1;
                }
            }
            
            std::filesystem::remove("/tmp/mxdbg_instr.bin");
            return 1;
        } catch (...) {
            return 1;
        }
    }
}