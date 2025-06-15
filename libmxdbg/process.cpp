#include "mxdbg/process.hpp"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

namespace mx {

    std::unique_ptr<Process> Process::launch(const std::filesystem::path& program, const std::vector<std::string> &args) {
        pid_t pid = fork();
        
        if (pid == 0) {
            if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
                throw std::runtime_error("Failed to enable tracing: " + std::string(strerror(errno)));
            }
            std::vector<char*> argv;
            argv.push_back(program.string().data());
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            execv(program.c_str(), argv.data());
            throw std::runtime_error("Failed to execute program: " + std::string(strerror(errno)));
        } else if (pid > 0) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                throw std::runtime_error("Failed to wait for child: " + std::string(strerror(errno)));
            }
            if (!WIFSTOPPED(status)) {
                throw std::runtime_error("Child process did not stop as expected");
            }
            std::unique_ptr<Process>  p(new Process(pid));
            return p;
        } else {
            throw std::runtime_error("Failed to fork: " + std::string(strerror(errno)));
        }
    }

    std::unique_ptr<Process> Process::attach(pid_t pid) {
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
            throw std::runtime_error("Failed to attach to process: " + std::string(strerror(errno)));
        }
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            throw std::runtime_error("Failed to wait for process: " + std::string(strerror(errno)));
        }
        return std::unique_ptr<Process>(new Process(pid));
    }

    void Process::continue_execution() {
        if (ptrace(PTRACE_CONT, m_pid, nullptr, 0) == -1) {
            throw std::runtime_error("Failed to continue process: " + std::string(strerror(errno)));
        }
    }

    void Process::wait_for_stop() {
        int status;
        if (waitpid(m_pid, &status, 0) == -1) {
            throw std::runtime_error("Failed to wait for process: " + std::string(strerror(errno)));
        }
    }

    bool Process::is_running() const {
        int status;
        pid_t result = waitpid(m_pid, &status, WNOHANG);
        if (result == 0) {
            return true; 
        } else if (result == m_pid) {
            return false; 
        } else {
            return false; 
        }
    }

    void Process::detach() {
        if (kill(m_pid, 0) == -1) {
            if (errno == ESRCH) {
                return;
            }
        }        
        if (ptrace(PTRACE_DETACH, m_pid, nullptr, 0) == -1) {
            throw std::runtime_error("Failed to detach from process: " + std::string(strerror(errno)));
        }
    }

    int Process::get_exit_status() {
        int status;
        pid_t result = waitpid(m_pid, &status, WNOHANG);
        if (result == m_pid) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return -WTERMSIG(status);  
            }
        }
        return -1; 
    }
}