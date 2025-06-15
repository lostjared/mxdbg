#include"mxdbg/process.hpp"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>

namespace mx {

    std::unique_ptr<Process> Process::launch(const std::filesystem::path &executable, const char *args) {
        pid_t pid = fork();
        if (pid < 0) {
            return nullptr;
        } else if (pid == 0) {
            execl(executable.c_str(), executable.c_str(), args, nullptr);
            _exit(EXIT_FAILURE);
        } else {
            auto process = std::make_unique<Process>();
            process->pid = pid;
            process->executable = executable.string();
            process->args.push_back(args);
            ptrace(PTRACE_ATTACH, pid, nullptr, nullptr);
            waitpid(pid, nullptr, 0);
            return process;
        }
    }

    std::unique_ptr<Process> Process::attach(pid_t pid) {
        if (kill(pid, 0) < 0) {
            return nullptr; 
        }
        auto process = std::make_unique<Process>();
        process->pid = pid;
        process->executable = "unknown"; 
        process->args.clear(); 
        ptrace(PTRACE_ATTACH, pid, nullptr, nullptr);
        waitpid(pid, nullptr, 0); 
        return process;
    }
     

}