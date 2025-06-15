#ifndef MXDBG_PROCESS_HPP
#define MXDBG_PROCESS_HPP

#include<sys/types.h>
#include<string>
#include<vector>
#include<memory>
#include<filesystem>

namespace mx {
    class Process {
    public:
        static std::unique_ptr<Process> launch(const std::filesystem::path &exe, const std::vector<std::string>& args = {});
        static std::unique_ptr<Process> attach(pid_t pid);
        void continue_execution();
        void wait_for_stop();
        pid_t get_pid() const { return m_pid; }
        bool is_running() const;
    private:
        Process(pid_t pid) : m_pid(pid) {}    
        pid_t m_pid;
    };

} 

#endif