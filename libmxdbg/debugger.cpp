#include "mxdbg/debugger.hpp"
#include "mxdbg/exception.hpp"
#include<iostream>
#include<cstdlib>
#include<sstream>
#include<vector>
#include<thread>
#include<unistd.h>
#include<iomanip>
#include<filesystem>
#include<fstream>

namespace mx {

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

    Debugger::Debugger() : p_id(-1) {}

    Debugger::~Debugger() {}

    void Debugger::setup_history() {
    }
    bool Debugger::attach(pid_t pid) {
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
        if (cmd == "continue" || cmd == "c") {
            if (process && process->is_running()) {
                try {
                    process->continue_execution();
                    process->wait_for_stop();
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
                    std::cout << "Register " << reg_name << ": 0x" << std::hex << value << " | " << std::dec << value << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error getting register " << reg_name << ": " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
            return true;

        } else if(cmd == "base") {
            if (process && process->is_running()) {
                print_address();
            } else {
                std::cout << "No process attached or running." << std::endl;
            }
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
                std::cout << "Memory at 0x" << std::hex << addr << ": ";
                for (auto byte : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)byte << " ";
                }
                std::cout << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error reading memory: " << e.what() << std::endl;
            }
            return true;
        }
        else if (cmd == "help" || cmd == "h") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  continue, c    - Continue process execution" << std::endl;
            std::cout << "  step, s        - Execute single instruction" << std::endl;
            std::cout << "  step N, s N    - Execute N instructions" << std::endl;
            std::cout << "  status, st     - Show process status" << std::endl;
            std::cout << "  help, h        - Show this help message" << std::endl;
            std::cout << "  quit, q, exit  - Exit debugger" << std::endl;
            return true;
        } else if (tokens.size() == 2 && (tokens[0] == "break" || tokens[0] == "b")) {
            if (process && process->is_running()) {
                uint64_t addr = std::stoull(tokens[1], nullptr, 0);
                try {
                    process->set_breakpoint(addr);
                    std::cout << "Breakpoint set at 0x" << std::hex << addr << std::dec << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error setting breakpoint: " << e.what() << std::endl;
                }
            } else {
                std::cout << "No process attached or running." << std::endl;
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
            const size_t instruction_size = 16; 
            std::vector<uint8_t> instruction_bytes = process->read_memory(rip, instruction_size);
            
            std::cout << "Current instruction at 0x" << std::hex << rip << ": ";
            for (size_t i = 0; i < std::min(instruction_bytes.size(), size_t(8)); ++i) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) 
                          << static_cast<unsigned>(instruction_bytes[i]) << " ";
            }
            std::cout << std::dec << std::endl;
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
            while (std::getline(file, line)) {
                if (line.find("r-xp") != std::string::npos && line.find(program_name) != std::string::npos) {
                    auto pos = line.find_first_of('-');
                    if (pos != std::string::npos) {
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
                std::cout << "Base address: 0x" << std::hex << base_address << std::dec << std::endl;
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
                std::cout << "Current PC: 0x" << std::hex << pc << std::dec << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error getting current PC: " << e.what() << std::endl;
            }
        } else {
            std::cout << "No process attached or running." << std::endl;
        }

    }
}