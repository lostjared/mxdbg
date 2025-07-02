/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef ___DEBUGGER__H__
#define ___DEBUGGER__H__

#include<iostream>
#include<memory>
#include<string>
#include<vector>
#include<map>
#include<filesystem>
#include<cstdlib>
#include"mxdbg/process.hpp"
#include<mx2-ollama.hpp>
namespace mx {

    std::vector<std::string> split_command(const std::string &cmd);
    std::string join(size_t start, size_t stop, std::vector<std::string>  &tokens, const std::string &delimiter);
    
    struct MemoryRegion {
        uint64_t start;
        uint64_t end;
        std::string permissions;
        std::string pathname;
        bool is_readable() const { return permissions[0] == 'r'; }
        bool is_writable() const { return permissions[1] == 'w'; }
        bool is_executable() const { return permissions[2] == 'x'; }
    };

    struct ThreadInfo {
        pid_t tid;
        std::string state;
        uint64_t rip;
        std::string name;
    };

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
        uint64_t expression(const std::string &text);
        
        void setup_history();
        void save_history();
        void detach();
        
        uint64_t get_base_address() const;
        bool setfunction_breakpoint(const std::string &function_name);
        uint64_t calculate_variable_address(const std::string &r, uint64_t value);
        void break_if(uint64_t location, const std::string &e);
        void step_over();     
        void step_out();      
        void run_until(uint64_t address);  
    private:
        void print_current_instruction(); 
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
        void print_backtrace() const;
        std::vector<uint64_t> get_stack_frames() const;
        std::string resolve_symbol(uint64_t address) const;
        bool is_at_function_entry() const;
        bool is_valid_code_address(uint64_t address) const;
        void analyze_current_frame() const;
        void print_memory_maps() const;    
        void search_memory_for_int32(int32_t value);
        void search_memory_for_int64(int64_t value);
        void search_memory_for_string(const std::string& pattern);
        void search_memory_for_bytes(const std::vector<std::string>& byte_tokens);
        void search_memory_for_pattern(const std::string& pattern);
        std::vector<MemoryRegion> get_searchable_memory_regions();
        bool match_pattern(const std::vector<uint8_t>& data, const std::string& pattern, size_t offset);
        std::vector<size_t> find_in_memory(const std::vector<uint8_t>& haystack, const std::vector<uint8_t>& needle);
        bool parse_pattern(const std::string& pattern, std::vector<std::pair<uint8_t, bool>>& parsed_pattern);
        std::vector<size_t> find_pattern_in_memory(const std::vector<uint8_t>& memory, const std::vector<std::pair<uint8_t, bool>>& pattern);
        void list_threads();
        void switch_thread(pid_t id);

    };

} 

#endif