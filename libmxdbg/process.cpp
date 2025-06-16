#include"mxdbg/process.hpp"
#include"mxdbg/pipe.hpp"
#include"mxdbg/exception.hpp"
#include<sys/ptrace.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdexcept>
#include<cstring>
#include<sstream>
#include<iostream>
#include<vector>
#include<thread>
#include<chrono>
#include<fstream>

namespace mx {
    
    Process::Process(Process&& proc) : m_pid(proc.m_pid) {
        proc.m_pid = -1; 
    }
   
    Process &Process::operator=(Process&& other) {
        if (this != &other) {
            m_pid = other.m_pid;
            other.m_pid = -1; 
        }
        return *this;
    }

    std::unique_ptr<Process> Process::launch(const std::filesystem::path& program, const std::vector<std::string> &args) {

        if (!std::filesystem::exists(program)) {
            throw mx::Exception("Executable file does not exist: " + program.string());
        }
        if (!std::filesystem::is_regular_file(program)) {
            throw mx::Exception("Path is not a regular file: " + program.string());
        }
        if (access(program.c_str(), X_OK) != 0) {
            throw mx::Exception("Executable file is not accessible: " + program.string());
        }
        
        mx::Pipe error_pipe;
        pid_t pid = fork();
        if (pid == 0) {
            error_pipe.close_read();          
            if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
                std::string error_msg = "PTRACE_FAILED: " + std::string(strerror(errno));
                error_pipe.write(error_msg);
                error_pipe.close_write();
                exit(1);
            }
            std::vector<char*> args_vec;
            args_vec.push_back(const_cast<char*>(program.filename().c_str()));
            for (const auto& arg : args) {
                args_vec.push_back(const_cast<char*>(arg.c_str()));
            }
            args_vec.push_back(nullptr);
            error_pipe.close_write();
            execvp(program.string().c_str(), args_vec.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            error_pipe.close_write();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::string error_msg = error_pipe.read_nonblocking();
            if (!error_msg.empty()) {
                error_pipe.close();
                kill(pid, SIGKILL);
                int status;
                waitpid(pid, &status, 0);
                throw mx::Exception("Failed to launch process: " + error_msg);
            }
            error_pipe.close();
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                throw mx::Exception::error("Failed to wait for child process");
            }
            if (!WIFSTOPPED(status)) {
                throw mx::Exception("Child process did not stop as expected");
            }
            return std::unique_ptr<Process>(new Process(pid));
        } else {
            throw mx::Exception::error("Failed to fork");
        }
    }

    std::unique_ptr<Process> Process::attach(pid_t pid) {
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
            throw mx::Exception::error("Failed to attach to process");
        }
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for process after attaching");
        }
        return std::unique_ptr<Process>(new Process(pid));
    }

    void Process::continue_execution() {
        if (ptrace(PTRACE_CONT, m_pid, nullptr, 0) == -1) {
            throw mx::Exception::error("Failed to continue process");
        }
    }

    void Process::wait_for_stop() {
        int status;
        if (waitpid(m_pid, &status, 0) == -1) {
            throw mx::Exception::error("Failed to wait for process stop");
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
            throw mx::Exception::error("Failed to detach from process");
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

    std::string Process::proc_info() const {
        std::ostringstream stream;
        std::filesystem::path proc_path = "/proc/" + std::to_string(m_pid);
        if (!std::filesystem::exists(proc_path)) {
            return "Process not found in procfs";
        }
        std::ifstream status_file(proc_path / "status");
        if (status_file.is_open()) {
            std::string line;
            while (std::getline(status_file, line)) {
                stream << line << "\n";
            }
            status_file.close();
        } else {
            stream << "Unable to read process status from " << proc_path.string() << "/status";
        }
        return stream.str();
    }
}