#include"mxdbg/exception.hpp"
#include"mxdbg/pipe.hpp"
#include<iostream>
#include<cassert>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<thread>
#include<chrono>
#include<sstream>
#include<exception>

void test_basic_exception_construction() {
    std::cout << "Testing basic exception construction..." << std::endl;
    
    try {
        throw mx::Exception("Test error message");
    } catch (const mx::Exception& e) {
        std::string message = e.what();
        assert(message == "Test error message");
    }
    
    try {
        throw mx::Exception("C-style error");
    } catch (const mx::Exception& e) {
        std::string message = e.what();
        assert(message == "C-style error");
    }
    
    std::cout << "╬ô┬ú├┤ Basic exception construction test passed" << std::endl;
}

void test_errno_based_exception() {
    std::cout << "Testing errno-based exception..." << std::endl;
    
    errno = ENOENT;
    
    try {
        throw mx::Exception::error("File operation failed");
    } catch (const mx::Exception& e) {
        std::string message = e.what();
        assert(message.find("File operation failed") != std::string::npos);
        assert(message.find("No such file or directory") != std::string::npos);
    }
    
    errno = EACCES;
    
    try {
        throw mx::Exception::error("Permission check");
    } catch (const mx::Exception& e) {
        std::string message = e.what();
        assert(message.find("Permission check") != std::string::npos);
        assert(message.find("Permission denied") != std::string::npos);
    }
    
    std::cout << "╬ô┬ú├┤ Errno-based exception test passed" << std::endl;
}

void test_exception_inheritance() {
    std::cout << "Testing exception inheritance..." << std::endl;
    
    try {
        throw mx::Exception("Inheritance test");
    } catch (const std::runtime_error& e) {
        std::string message = e.what();
        assert(message == "Inheritance test");
    }
    
    try {
        throw mx::Exception("Base exception test");
    } catch (const std::exception& e) {
        std::string message = e.what();
        assert(message == "Base exception test");
    }
    
    std::cout << "╬ô┬ú├┤ Exception inheritance test passed" << std::endl;
}

std::string serialize_exception(const mx::Exception& e) {
    return std::string("EXCEPTION:") + e.what();
}

mx::Exception deserialize_exception(const std::string& data) {
    if (data.substr(0, 10) == "EXCEPTION:") {
        return mx::Exception(data.substr(10));
    }
    throw std::runtime_error("Invalid exception data");
}

void test_exception_through_pipe_same_process() {
    std::cout << "Testing exception through pipe (same process)..." << std::endl;
    
    mx::Pipe pipe;
    
    mx::Exception original_exception("Test exception through pipe");
    std::string serialized = serialize_exception(original_exception);
    
    pipe.write(serialized);
    pipe.close_write();
    
    std::string received = pipe.read();
    mx::Exception deserialized = deserialize_exception(received);
    
    assert(std::string(deserialized.what()) == std::string(original_exception.what()));
    
    std::cout << "╬ô┬ú├┤ Exception through pipe (same process) test passed" << std::endl;
}

void test_exception_through_pipe_different_processes() {
    std::cout << "Testing exception through pipe (different processes)..." << std::endl;
    
    mx::Pipe pipe;
    pid_t pid = fork();
    
    if (pid == 0) {
        pipe.close_read();
        
        try {
            int fd = open("/nonexistent/path/file.txt", O_RDONLY);
            if (fd == -1) {
                throw mx::Exception::error("Failed to open file");
            }
        } catch (const mx::Exception& e) {
            std::string serialized = serialize_exception(e);
            pipe.write(serialized);
        }
        
        pipe.close_write();
        exit(0);
        
    } else if (pid > 0) {
        pipe.close_write();
        
        std::string received = pipe.read();
        mx::Exception received_exception = deserialize_exception(received);
        
        std::string message = received_exception.what();
        assert(message.find("Failed to open file") != std::string::npos);
        assert(message.find("No such file or directory") != std::string::npos);
        
        int status;
        waitpid(pid, &status, 0);
        
        std::cout << "  Child sent exception: " << message << std::endl;
        
    } else {
        throw std::runtime_error("Failed to fork");
    }
    
    std::cout << "╬ô┬ú├┤ Exception through pipe (different processes) test passed" << std::endl;
}

void test_multiple_exceptions_through_pipe() {
    std::cout << "Testing multiple exceptions through pipe..." << std::endl;
    
    mx::Pipe pipe;
    
    std::vector<mx::Exception> exceptions = {
        mx::Exception("First error"),
        mx::Exception("Second error"),
        mx::Exception("Third error with details")
    };
    
    for (const auto& exc : exceptions) {
        std::string serialized = serialize_exception(exc) + "\n";
        pipe.write(serialized);
    }
    pipe.close_write();
    
    std::string all_data = pipe.read();
    std::istringstream stream(all_data);
    std::string line;
    
    size_t count = 0;
    while (std::getline(stream, line) && !line.empty()) {
        mx::Exception received = deserialize_exception(line);
        assert(std::string(received.what()) == std::string(exceptions[count].what()));
        count++;
    }
    
    assert(count == exceptions.size());
    
    std::cout << "╬ô┬ú├┤ Multiple exceptions through pipe test passed" << std::endl;
}

void test_exception_with_pipe_errors() {
    std::cout << "Testing exception handling with pipe errors..." << std::endl;
    
    mx::Pipe pipe;
    
    pipe.close_read();
    
    try {
        pipe.read();
        assert(false && "Should have thrown exception");
    } catch (const mx::Exception& e) {
        std::string message = e.what();
        assert(message.find("closed for reading") != std::string::npos);
        std::cout << "  Caught expected pipe exception: " << message << std::endl;
    } catch (const std::runtime_error& e) {
        std::string message = e.what();
        assert(message.find("closed") != std::string::npos);
        std::cout << "  Caught expected runtime_error: " << message << std::endl;
    }
    
    std::cout << "╬ô┬ú├┤ Exception handling with pipe errors test passed" << std::endl;
}

void test_exception_error_context() {
    std::cout << "Testing exception error context..." << std::endl;
    
    std::vector<std::pair<int, std::string>> test_cases = {
        {ENOENT, "File not found"},
        {EACCES, "Access denied"},
        {ENOMEM, "Out of memory"},
        {EINVAL, "Invalid argument"}
    };
    
    for (const auto& test_case : test_cases) {
        errno = test_case.first;
        
        try {
            throw mx::Exception::error(test_case.second);
        } catch (const mx::Exception& e) {
            std::string message = e.what();
            assert(message.find(test_case.second) != std::string::npos);
            assert(message.find(std::strerror(test_case.first)) != std::string::npos);
        }
    }
    
    std::cout << "╬ô┬ú├┤ Exception error context test passed" << std::endl;
}

void test_threaded_exception_communication() {
    std::cout << "Testing threaded exception communication..." << std::endl;
    
    mx::Pipe pipe;
    bool thread_completed = false;
    std::string error_message;
    
    std::thread producer([&pipe, &thread_completed, &error_message]() {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            errno = ETIMEDOUT;
            throw mx::Exception::error("Thread operation timed out");
            
        } catch (const mx::Exception& e) {
            std::string serialized = serialize_exception(e);
            pipe.write(serialized);
            pipe.close_write();
            thread_completed = true;
        } catch (const std::exception& e) {
            error_message = e.what();
            pipe.close_write();
            thread_completed = true;
        }
    });
    
    producer.join();
    
    if (!thread_completed) {
        throw std::runtime_error("Thread did not complete properly");
    }
    
    if (!error_message.empty()) {
        throw std::runtime_error("Thread failed with error: " + error_message);
    }
    
    std::string received = pipe.read();
    if (received.empty()) {
        throw std::runtime_error("No data received from thread");
    }
    
    mx::Exception received_exception = deserialize_exception(received);
    
    std::string message = received_exception.what();
    assert(message.find("Thread operation timed out") != std::string::npos);
    
    std::cout << "  Received threaded exception: " << message << std::endl;
    std::cout << "Γ£ô Threaded exception communication test passed" << std::endl;
}

int main() {
    std::cout << "Running comprehensive exception tests...\n" << std::endl;
    
    try {
        test_basic_exception_construction();
        test_errno_based_exception();
        test_exception_inheritance();
        test_exception_through_pipe_same_process();
        test_exception_through_pipe_different_processes();
        test_multiple_exceptions_through_pipe();
        test_exception_with_pipe_errors();
        test_exception_error_context();
        test_threaded_exception_communication();
        
        std::cout << "\nΓëí╞Æ├ä├½ All exception tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n╬ô┬Ñ├« Test failed: " << e.what() << std::endl;
        return 1;
    }
}