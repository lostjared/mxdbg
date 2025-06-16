#include <mxdbg/pipe.hpp>
#include <mxdbg/exception.hpp>
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

void test_write_functions() {
    std::cout << "Testing all write functions..." << std::endl;
    
    {
        mx::Pipe pipe;
        std::string test_string = "Hello, World!";
        size_t bytes_written = pipe.write(test_string);
        assert(bytes_written == test_string.size());
        
        std::string result = pipe.read();
        assert(result == test_string);
        std::cout << "  String write test passed" << std::endl;
    }
    
    {
        mx::Pipe pipe;
        std::string original = "Binary data test";
        const std::byte* byte_data = reinterpret_cast<const std::byte*>(original.c_str());
        size_t bytes_written = pipe.write(byte_data, original.size());
        assert(bytes_written == original.size());
        
        std::string result = pipe.read();
        assert(result == original);
        std::cout << "  Byte pointer write test passed" << std::endl;
    }
    
    {
        mx::Pipe pipe;
        std::string original = "Vector byte test";
        std::vector<std::byte> byte_vector;
        for (char c : original) {
            byte_vector.push_back(static_cast<std::byte>(c));
        }
        
        size_t bytes_written = pipe.write(byte_vector);
        assert(bytes_written == byte_vector.size());
        
        std::string result = pipe.read();
        assert(result == original);
        std::cout << "  Byte vector write test passed" << std::endl;
    }
    
    {
        mx::Pipe pipe;
        
        size_t bytes_written = pipe.write(std::string());
        assert(bytes_written == 0);
        
        std::vector<std::byte> empty_vector;
        bytes_written = pipe.write(empty_vector);
        assert(bytes_written == 0);
        
        bytes_written = pipe.write(nullptr, 0);
        assert(bytes_written == 0);
        
        std::cout << "  Empty data write tests passed" << std::endl;
    }
    
    {
        mx::Pipe pipe;
        std::vector<std::byte> binary_data = {
            std::byte{0x00}, std::byte{0x48}, std::byte{0x65}, std::byte{0x6C}, 
            std::byte{0x6C}, std::byte{0x6F}, std::byte{0x00}, std::byte{0xFF}
        };
        
        size_t bytes_written = pipe.write(binary_data);
        assert(bytes_written == binary_data.size());
        
        std::vector<std::byte> result = pipe.read_bytes();
        assert(result == binary_data);
        std::cout << "  Binary data with nulls test passed" << std::endl;
    }
    
    std::cout << " All write functions tests passed" << std::endl;
}

void test_large_write_functions() {
    std::cout << "Testing large data with all write functions..." << std::endl;
    
    const size_t large_size = 50000;
    
    {
        mx::Pipe pipe;
        std::string large_string(large_size, 'X');
        for (size_t i = 0; i < large_size; i += 1000) {
            large_string[i] = 'Y';
        }
        
        std::thread writer([&pipe, &large_string]() {
            size_t bytes_written = pipe.write(large_string);
            assert(bytes_written == large_string.size());
            pipe.close_write();
        });
        
        std::string result;
        std::string chunk;
        while (!(chunk = pipe.read()).empty()) {
            result += chunk;
        }
        
        writer.join();
        assert(result == large_string);
        std::cout << "  Large string write test passed" << std::endl;
    }
    
    {
        mx::Pipe pipe;
        std::vector<std::byte> large_vector(large_size);
        for (size_t i = 0; i < large_size; ++i) {
            large_vector[i] = static_cast<std::byte>(i % 256);
        }
        
        std::thread writer([&pipe, &large_vector]() {
            size_t bytes_written = pipe.write(large_vector);
            assert(bytes_written == large_vector.size());
            pipe.close_write();
        });
        
        std::vector<std::byte> result;
        std::vector<std::byte> chunk;
        while (!(chunk = pipe.read_bytes()).empty()) {
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        
        writer.join();
        assert(result == large_vector);
        std::cout << "  Large byte vector write test passed" << std::endl;
    }
    
    std::cout << " Large write functions tests passed" << std::endl;
}

void test_write_error_conditions() {
    std::cout << "Testing write error conditions..." << std::endl;
    
    {
        mx::Pipe pipe;
        pipe.close_write();
        
        try {
            pipe.write(std::string("test"));
            assert(false && "Should have thrown exception");
        } catch (const std::exception& e) {
            std::cout << "  Expected string write error: " << e.what() << std::endl;
        }
        
        try {
            std::vector<std::byte> data = {std::byte{0x01}};
            pipe.write(data);
            assert(false && "Should have thrown exception");
        } catch (const std::exception& e) {
            std::cout << "  Expected byte vector write error: " << e.what() << std::endl;
        }
        
        try {
            std::byte data = std::byte{0x01};
            pipe.write(&data, 1);
            assert(false && "Should have thrown exception");
        } catch (const std::exception& e) {
            std::cout << "  Expected byte pointer write error: " << e.what() << std::endl;
        }
    }
    
    std::cout << " Write error conditions tests passed" << std::endl;
}

void test_large_data_transfer() {
    std::cout << "Testing large data transfer..." << std::endl;
    mx::Pipe pipe;
    std::string large_data(100000, 'A');
    for (size_t i = 0; i < large_data.size(); i += 100) {
        large_data[i] = 'B'; 
    }
    std::thread writer([&pipe, &large_data]() {
        pipe.write(large_data);
        pipe.close_write(); 
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
        throw mx::Exception("Failed to fork");
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
        test_write_functions();
        test_large_write_functions();
        test_write_error_conditions();
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