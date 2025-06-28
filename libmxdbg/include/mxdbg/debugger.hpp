#ifndef ___DEBUGGER__H__
#define ___DEBUGGER__H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include "mxdbg/process.hpp"
#include<mx2-ollama.hpp>
namespace mx {

    std::vector<std::string> split_command(const std::string &cmd);
    std::string join(size_t start, size_t stop, std::vector<std::string>  &tokens, const std::string &delimiter);
    
    class Debugger {
    public:
        Debugger(bool ai = true);
        ~Debugger();

        bool attach(pid_t pid);
        bool launch(const std::filesystem::path &exe, std::string_view args);
        void dump_file(const std::filesystem::path &file);
        void print_address() const;
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
        
        uint64_t get_base_address() const;
        bool setfunction_breakpoint(const std::string &function_name);
    private:
        void print_current_instruction(); 
        std::size_t get_instruction_length(const std::vector<uint8_t>& bytes, size_t offset);
        std::unique_ptr<Process> process;
        std::string_view args;
        pid_t p_id;
        std::string history_filename;
        std::string program_name;
        std::string_view args_string;
        std::unique_ptr<mx::ObjectRequest> request;
        std::string obj_dump();
        std::string user_mode = "programmer";
        std::ostringstream code;
        std::string functionText(const std::string &text);
    };

} 

#endif