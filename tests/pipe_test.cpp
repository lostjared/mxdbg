#include "mxdbg/pipe.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include <unistd.h>
#include <sys/wait.h>

void test_basic_construction() {
    std::cout << "Testing basic construction..." << std::endl;
    mx::Pipe pipe;
    assert(pipe.is_readable());
    assert(pipe.is_writable());
    assert(pipe.get_read_fd() != -1);
    assert(pipe.get_write_fd() != -1);
    assert(pipe.get_read_fd() != pipe.get_write_fd());
    std::cout << " Basic construction test passed" << std::endl;
}

void test_move_semantics() {
    std::cout << "Testing move semantics..." << std::endl;
    mx::Pipe pipe1;
    int original_read_fd = pipe1.get_read_fd();
    int original_write_fd = pipe1.get_write_fd();
    mx::Pipe pipe2 = std::move(pipe1);
    assert(pipe2.get_read_fd() == original_read_fd);
    assert(pipe2.get_write_fd() == original_write_fd);
    assert(pipe1.get_read_fd() == -1);
    assert(pipe1.get_write_fd() == -1);
    assert(!pipe1.is_readable());
    assert(!pipe1.is_writable());
    mx::Pipe pipe3;
    pipe3 = std::move(pipe2);
    assert(pipe3.get_read_fd() == original_read_fd);
    assert(pipe3.get_write_fd() == original_write_fd);
    assert(pipe2.get_read_fd() == -1);
    assert(pipe2.get_write_fd() == -1);
    std::cout << " Move semantics test passed" << std::endl;
}

void test_basic_read_write() {
    std::cout << "Testing basic read/write..." << std::endl;
    mx::Pipe pipe;
    std::string test_data = "Hello, World!";
    pipe.write(test_data);
    std::string result = pipe.read();
    assert(result == test_data);
    
    std::cout << " Basic read/write test passed" << std::endl;
}

void test_large_data_transfer() {
    std::cout << "Testing large data transfer..." << std::endl;
    mx::Pipe pipe;
    std::string large_data(100000, 'A');
    for (size_t i = 0; i < large_data.size(); i += 100) {
        large_data[i] = 'B'; // Add some variation
    }
    std::thread writer([&pipe, &large_data]() {
        pipe.write(large_data);
        pipe.close_write(); // Signal EOF
    });
    std::string result;
    std::string chunk;
    while (!(chunk = pipe.read()).empty()) {
        result += chunk;
    }
    
    writer.join();
    assert(result == large_data);
    
    std::cout << " Large data transfer test passed" << std::endl;
}

void test_multiple_writes_reads() {
    std::cout << "Testing multiple writes/reads..." << std::endl;    
    mx::Pipe pipe;
    std::vector<std::string> test_strings = {
        "First message",
        "Second message", 
        "Third message with numbers: 12345",
        "Final message!"
    };
    size_t expected_length = 0;
    std::string expected;
    for (const auto& msg : test_strings) {
        expected += msg;
        expected_length += msg.length();
    }
    for (const auto& msg : test_strings) {
        pipe.write(msg);
    }
    pipe.close_write();   
    std::string all_data;
    std::string chunk;
    while (!(chunk = pipe.read()).empty()) {
        all_data += chunk;
    }
    assert(all_data == expected);
    std::cout << " Multiple writes/reads test passed" << std::endl;
}

void test_close_operations() {
    std::cout << "Testing close operations..." << std::endl;
    mx::Pipe pipe;
    assert(pipe.is_readable());
    assert(pipe.is_writable());
    
    pipe.close_write();
    assert(pipe.is_readable());
    assert(!pipe.is_writable());
    
    pipe.close_read();
    assert(!pipe.is_readable());
    assert(!pipe.is_writable());
    mx::Pipe pipe2;
    pipe2.close();
    assert(!pipe2.is_readable());
    assert(!pipe2.is_writable());
    
    std::cout << " Close operations test passed" << std::endl;
}

void test_error_conditions() {
    std::cout << "Testing error conditions..." << std::endl;
    mx::Pipe pipe;
    pipe.close_write();
    try {
        pipe.write("This should fail");
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected error: " << e.what() << std::endl;
    }
    pipe.close_read();
    try {
        pipe.read();
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected error: " << e.what() << std::endl;
    }
    
    std::cout << " Error conditions test passed" << std::endl;
}

void test_eof_behavior() {
    std::cout << "Testing EOF behavior..." << std::endl;
    mx::Pipe pipe;
    pipe.write("Hello");
    pipe.close_write();
    std::string result = pipe.read();
    assert(result == "Hello");
    result = pipe.read();
    assert(result.empty());
    
    std::cout << " EOF behavior test passed" << std::endl;
}

void test_interprocess_communication() {
    std::cout << "Testing interprocess communication..." << std::endl;
    mx::Pipe pipe;
    pid_t pid = fork();
    if (pid == 0) {
        pipe.close_read(); 
        std::string message = "Message from child process";
        pipe.write(message);
        pipe.close_write();
        exit(0);
    } else if (pid > 0) {
        pipe.close_write(); 
        std::string result = pipe.read();
        assert(result == "Message from child process");
        int status;
        waitpid(pid, &status, 0);
        
        std::cout << " Interprocess communication test passed" << std::endl;
    } else {
        throw std::runtime_error("Failed to fork");
    }
}

void test_nonblocking_read() {
    std::cout << "Testing non-blocking read..." << std::endl;
    mx::Pipe pipe;
    std::string result = pipe.read_nonblocking();
    assert(result.empty());
    pipe.write("Test data");
    result = pipe.read_nonblocking();
    assert(result == "Test data");
    result = pipe.read_nonblocking();
    assert(result.empty());
    std::cout << " Non-blocking read test passed" << std::endl;
}

void test_destructor_cleanup() {
    std::cout << "Testing destructor cleanup..." << std::endl;
    int read_fd, write_fd;
    {
        mx::Pipe pipe;
        read_fd = pipe.get_read_fd();
        write_fd = pipe.get_write_fd();
        assert(read_fd != -1);
        assert(write_fd != -1);
    } 
    char test = 'x';
    assert(::write(write_fd, &test, 1) == -1);
    assert(::read(read_fd, &test, 1) == -1);
    
    std::cout << " Destructor cleanup test passed" << std::endl;
}

int main() {
    std::cout << "Running comprehensive pipe tests...\n" << std::endl;
    
    try {
        test_basic_construction();
        test_move_semantics();
        test_basic_read_write();
        test_large_data_transfer();
        test_multiple_writes_reads();
        test_close_operations();
        test_error_conditions();
        test_eof_behavior();
        test_interprocess_communication();
        test_nonblocking_read();
        test_destructor_cleanup();
        
        std::cout << "\n All pipe tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n Test failed: " << e.what() << std::endl;
        return 1;
    }
}