/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef MXDBG_PROCESS_HPP
#define MXDBG_PROCESS_HPP

#include<sys/types.h>
#include<string>
#include<vector>
#include<map>
#include<memory>
#include<filesystem>
#include<sys/user.h>
#include<unistd.h>

namespace mx {

    enum class WatchType {
        READ = 1,
        WRITE = 2,
        ACCESS = 3  
    };

    struct Watchpoint {
        uint64_t address;
        size_t size;
        WatchType type;
        std::string description;
        std::vector<uint8_t> instruction_bytes;  
        std::string disassembly;  
    };

    class Process {
    public:
        Process(const Process&) = delete;
        ~Process() = default; 
        Process& operator=(const Process&) = delete;
        Process(Process&&);
        Process& operator=(Process&&);
        static std::unique_ptr<Process> launch(const std::filesystem::path &exe, const std::vector<std::string> &args = {});
        static std::unique_ptr<Process> attach(pid_t pid);
        void continue_execution();
        void wait_for_stop();
        pid_t get_pid() const { return m_pid; }
        bool is_running() const;
        void detach();
        int get_exit_status();
        std::string proc_info() const;
        std::string reg_info() const;
        void single_step();
        void wait_for_single_step();
        uint64_t get_register(const std::string &reg_name) const;
        uint32_t get_register_32(const std::string &reg_name) const;
        uint16_t get_register_16(const std::string &reg_name) const;
        uint8_t get_register_8(const std::string &reg_name) const;
        std::vector<std::string> get_all_registers() const;
        void print_all_registers() const;
        void set_register(const std::string &reg_name, uint64_t value);
        void set_register_32(const std::string &reg_name, uint32_t value);
        void set_register_16(const std::string &reg_name, uint16_t value);
        void set_register_8(const std::string &reg_name, uint8_t value);
        std::vector<uint8_t> read_memory(uint64_t address, size_t size) const;
        void write_memory(uint64_t address, const std::vector<uint8_t> &data);
        uint64_t get_pc() const;
        std::string disassemble_instruction(uint64_t address) const;
        std::string get_current_instruction() const;
        void set_breakpoint(uint64_t address);
        bool remove_breakpoint(uint64_t address);
        bool remove_breakpoint_by_index(size_t index);
        bool has_breakpoint(uint64_t address) const;
        std::vector<std::pair<uint64_t, uint64_t>> get_breakpoints() const;
        std::vector<std::pair<size_t, uint64_t>> get_breakpoints_with_index() const;
        uint64_t get_breakpoint_address_by_index(size_t index) const;
        size_t get_breakpoint_index_by_address(uint64_t address) const;
        uint8_t get_original_instruction(uint64_t address) const;
        void handle_breakpoint_continue(uint64_t address);
        bool set_watchpoint(uint64_t address, size_t size, WatchType type);
        bool remove_watchpoint(uint64_t address);
        std::vector<Watchpoint> get_watchpoints() const;
        bool has_watchpoint_at(uint64_t address) const;
        std::string disassemble_instruction(uint64_t address, const std::vector<uint8_t>& bytes);
        void switch_to_thread(pid_t id);
        pid_t get_current_thread() const;
        std::vector<pid_t> get_thread_ids() const;
        bool is_valid_thread(pid_t tid) const;
        void handle_thread_breakpoint_continue(uint64_t address);
    private:
        Process(pid_t pid) : m_pid(pid), current_thread_id(pid), index_(0) {}    
        pid_t m_pid, current_thread_id;
        bool is_single_stepping = false;
        std::map<uint64_t, uint8_t> breakpoints;
        std::vector<uint64_t> breakpoint_index;   
        void handle_breakpoint_step(uint64_t address);
        void set_pc(uint64_t address);    
        
        size_t index_;    
        std::vector<Watchpoint> watchpoints_;
    };  

} 

#endif