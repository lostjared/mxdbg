#include "mxdbg/process.hpp"
#include "mxdbg/exception.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

pid_t launch_dummy_process() {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: just sleep
        execl("/bin/sleep", "sleep", "60", nullptr);
        exit(1);
    }
    
    // Give child time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return pid;
}

void test_all_registers(mx::Process& process) {
    std::cout << "Testing 64-bit registers..." << std::endl;
    std::map<std::string, uint64_t> regs64 = {
        {"rax", 0x1122334455667788}, {"rbx", 0x8877665544332211},
        {"rcx", 0xabcdef0123456789}, {"rdx", 0x9876543210fedcba},
        {"rsi", 0x1234567890abcdef}, {"rdi", 0xfedcba0987654321}
    };

    for (const auto& pair : regs64) {
        std::cout << "  Testing " << pair.first << "..." << std::endl;
        process.set_register(pair.first, pair.second);
        uint64_t value = process.get_register(pair.first);
        assert(value == pair.second);
    }
    std::cout << "64-bit register tests passed.\n" << std::endl;

    std::cout << "Testing 32-bit registers..." << std::endl;
    std::map<std::string, uint32_t> regs32 = {
        {"eax", 0x12345678}, {"ebx", 0x87654321},
        {"ecx", 0xabcdef12}, {"edx", 0x12fedcba}
    };

    for (const auto& pair : regs32) {
        std::cout << "  Testing " << pair.first << "..." << std::endl;
        process.set_register_32(pair.first, pair.second);
        uint32_t value = process.get_register_32(pair.first);
        assert(value == pair.second);
    }
    std::cout << "32-bit register tests passed.\n" << std::endl;

    std::cout << "Testing 16-bit registers..." << std::endl;
    std::map<std::string, uint16_t> regs16 = {
        {"ax", 0x1234}, {"bx", 0x5678},
        {"cx", 0xabcd}, {"dx", 0xef12}
    };

    for (const auto& pair : regs16) {
        std::cout << "  Testing " << pair.first << "..." << std::endl;
        process.set_register_16(pair.first, pair.second);
        uint16_t value = process.get_register_16(pair.first);
        assert(value == pair.second);
    }
    std::cout << "16-bit register tests passed.\n" << std::endl;

    std::cout << "Testing 8-bit registers..." << std::endl;
    std::map<std::string, uint8_t> regs8 = {
        {"al", 0x12}, {"ah", 0x34},
        {"bl", 0x56}, {"bh", 0x78},
        {"cl", 0xab}, {"ch", 0xcd},
        {"dl", 0xef}, {"dh", 0x12}
    };

    for (const auto& pair : regs8) {
        std::cout << "  Testing " << pair.first << "..." << std::endl;
        process.set_register_8(pair.first, pair.second);
        uint8_t value = process.get_register_8(pair.first);
        assert(value == pair.second);
    }
    std::cout << "8-bit register tests passed.\n" << std::endl;
}

int main() {
    std::cout << "Starting register read/write test..." << std::endl;
    
    std::cout << "Launching dummy process..." << std::endl;
    pid_t child_pid = launch_dummy_process();
    if (child_pid <= 0) {
        std::cerr << "Failed to launch dummy process." << std::endl;
        return 1;
    }
    std::cout << "Dummy process launched with PID: " << child_pid << std::endl;

    std::unique_ptr<mx::Process> process = nullptr;
    try {
        std::cout << "About to attach to process " << child_pid << "..." << std::endl;
        process = mx::Process::attach(child_pid);
        std::cout << "Successfully attached." << std::endl;

        std::cout << "About to test registers..." << std::endl;
        test_all_registers(*process);

        std::cout << "\nAll register tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        if (process) {
            std::cout << "Detaching process..." << std::endl;
            process->detach();
        }
        std::cout << "Killing child process..." << std::endl;
        kill(child_pid, SIGKILL);
        return 1;
    }

    // Cleanup
    std::cout << "Cleaning up..." << std::endl;
    if (process) {
        std::cout << "Detaching process..." << std::endl;
        process->detach();
    }
    std::cout << "Killing child process..." << std::endl;
    kill(child_pid, SIGKILL);
    waitpid(child_pid, nullptr, 0);
    std::cout << "Test finished and cleaned up." << std::endl;

    return 0;
}