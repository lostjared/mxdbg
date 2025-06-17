#ifndef ___DEBUGGER__H__
#define ___DEBUGGER__H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include "mxdbg/process.hpp"

namespace mx {

    std::vector<std::string> split_command(const std::string &cmd);
    std::string join(size_t start, size_t stop, std::vector<std::string>  &tokens, const std::string &delimiter);
    
    class Debugger {
    public:
        Debugger();
        ~Debugger();

        bool attach(pid_t pid);
        bool launch(const std::filesystem::path &exe, std::string_view args);
        
        void continue_execution();
        void wait_for_stop();
        void step();
        void step_n(int count);
        
        pid_t get_pid() const;
        bool is_running() const;
        
        bool command(const std::string &cmd);
        
        void setup_history();
        void save_history();
        void detach();

    private:
        void print_current_instruction(); 
        std::unique_ptr<Process> process;
        std::string_view args;
        pid_t p_id;
        std::string history_filename;
    };

} 

#endif