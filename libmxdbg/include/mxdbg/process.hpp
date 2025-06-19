#ifndef MXDBG_PROCESS_HPP
#define MXDBG_PROCESS_HPP

#include<sys/types.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
#include<filesystem>
#include<sys/user.h>
#include<unistd.h>


namespace mx {
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
        void remove_breakpoint(uint64_t address);
        bool has_breakpoint(uint64_t address) const;
        std::vector<uint64_t> get_breakpoints() const;
    private:
        Process(pid_t pid) : m_pid(pid) {}    
        pid_t m_pid;
        bool is_single_stepping = false;
        std::unordered_map<uint64_t, uint8_t> breakpoints;         
        void handle_breakpoint_continue(uint64_t address);
        void handle_breakpoint_step(uint64_t address);
        void set_pc(uint64_t address);        
    };  

} 

#endif