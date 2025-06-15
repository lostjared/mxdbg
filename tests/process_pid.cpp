#include "mxdbg/process.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <chrono>

pid_t launch_target_process() {
    pid_t pid = fork(); 
    if (pid == 0) {
        std::cout << "Child process " << getpid() << " starting infinite loop..." << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else if (pid > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Parent: launched child with PID " << pid << std::endl;
        return pid;
    } else {
        throw std::runtime_error("Failed to fork target process");
    }
}

bool process_exists(pid_t pid) {
    return kill(pid, 0) == 0;
}

int main(int argc, char* argv[]) {
    try {
        pid_t target_pid = 0;
        bool launched_process = false;
        
        if (argc > 1) {
            target_pid = std::stoi(argv[1]);            
            if (!process_exists(target_pid)) {
                std::cout << "Process with PID " << target_pid << " not found. Launching new process..." << std::endl;
                target_pid = 0;
            } else {
                std::cout << "Attaching to existing process with PID: " << target_pid << std::endl;
            }
        }
        
        if (target_pid == 0) {
            std::cout << "Launching target process..." << std::endl;
            target_pid = launch_target_process();
            launched_process = true;
            std::cout << "Launched target process with PID: " << target_pid << std::endl;
        }
        
        if (!process_exists(target_pid)) {
            throw std::runtime_error("Target process died before we could attach");
        }
        
        std::cout << "Attaching debugger to process " << target_pid << "..." << std::endl;
        auto process = mx::Process::attach(target_pid);
        std::cout << "Successfully attached to process!" << std::endl;
        std::cout << "Process is now paused." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "Continuing process execution..." << std::endl;
        process->continue_execution();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        if (!process_exists(target_pid)) {
            std::cout << "Warning: Process terminated unexpectedly after continue_execution()" << std::endl;
        } else {
            std::cout << "Process is still running after continue_execution()" << std::endl;
        }
        
        if (launched_process) {
            if (process_exists(target_pid)) {
                std::cout << "detaching debugger from process " << target_pid << "..." << std::endl;
                try {
                    process->detach();
                    std::cout << "Debugger detached successfully." << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Detach failed (process may have terminated): " << e.what() << std::endl;
                }
                
                if (process_exists(target_pid)) {
                    std::cout << "Terminating launched process..." << std::endl;
                    kill(target_pid, SIGKILL); 
                    int status;
                    waitpid(target_pid, &status, 0);  
                    std::cout << "Process terminated successfully." << std::endl;
                } else {
                    std::cout << "Process already terminated." << std::endl;
                }
            } else {
                std::cout << "Process already terminated - no need to detach or kill." << std::endl;
            }
        } else {
            if (process_exists(target_pid)) {
                std::cout << "Detaching from external process..." << std::endl;
                try {
                    process->detach();
                } catch (const std::exception& e) {
                    std::cout << "Detach failed: " << e.what() << std::endl;
                }
            }
            std::cout << "Process continues running (not terminating external process)." << std::endl;
        }
       
        while (waitpid(-1, nullptr, WNOHANG) > 0) {
        }
        
        std::cout << "Test passed!" << std::endl;
        std::cout.flush();  
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        std::cerr.flush();
        return 1;
    }
}