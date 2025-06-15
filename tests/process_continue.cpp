#include<mxdbg/process.hpp>
#include<iostream>

int main(int argc, char **argv) {
    try {
        auto p = mx::Process::launch("/bin/ls");
        if (!p) {
            std::cerr << "Failed to launch process" << std::endl;
            return 1;
        }
        std::cout << "Process launched with PID: " << p->get_pid() << std::endl;
        p->continue_execution();
        p->wait_for_stop();
        if (p->is_running()) {
            std::cout << "Process is still running." << std::endl;
        } else {
            std::cout << "Process has stopped." << std::endl;
        }
        std::cout << "Process PID: " << p->get_pid() << std::endl;          
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
