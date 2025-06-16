#include "mxdbg/debugger.hpp"
#include <iostream>
#include <cstdlib>
#include<sstream>
#include<vector>
#include<thread>
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
                std::cerr << "Failed to attach to process with PID: " << pid << std::endl;
                return false;
            }
            p_id = pid;
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
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error launching process " << exe << ": " << e.what() << std::endl;
            return false;
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
            return true; 
        }
        if (tokens[0] == "continue") {
            continue_execution();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return true;
        } else if(tokens[0] == "status") {
            if (process) {
                std::cout << "Process PID: " << process->get_pid() << std::endl;
                std::cout << "Process is running: " << (process->is_running() ? "Yes" : "No") << std::endl;
            } else {
                std::cout << "No process attached or launched." << std::endl;
            }
            return true;
        } else if(tokens[0] == "info") {
            if(process && process->is_running()) {
                std::cout << process->proc_info() << std::endl;
            } else {
                std::cout << "No process attached or launched." << std::endl;
            }
            return true;
        } else if (tokens[0] == "exit" || tokens[0] == "quit") {
            if (process) {
                if(process->is_running()) {
                    process->detach();
                    std::cout << "Detached from process with PID: " << process->get_pid() << std::endl;
                }
            }
            return false; 
        } 
        std::cout << "Unknown command: " << tokens[0] << std::endl;
        return true;
    }
} 