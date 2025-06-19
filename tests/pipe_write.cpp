#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "mxdbg/pipe.hpp"

void test_blocking_write() {
    std::cout << "Testing blocking write...\n";
    
    mx::Pipe pipe;
    std::string data(1024, 'A');
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::size_t bytes_written = pipe.write(data);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Blocking write: " << bytes_written << " bytes written in " 
              << duration.count() << "ms\n";
}

void test_nonblocking_write() {
    std::cout << "Testing non-blocking write...\n";
    
    mx::Pipe pipe;
    
    std::vector<std::byte> large_data(65536);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<std::byte>('B');
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::size_t total_written = 0;
    std::size_t bytes_written = 0;
    
    do {
        try {
            bytes_written = pipe.write_nonblocking(large_data.data() + total_written, 
                                                   large_data.size() - total_written);
            if (bytes_written > 0) {
                total_written += bytes_written;
                std::cout << "Non-blocking write: " << bytes_written << " bytes written\n";
            }
            
            if (total_written >= large_data.size()) {
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
        } catch (const std::exception& e) {
            std::cout << "Write error: " << e.what() << "\n";
            break;
        }
        
    } while (total_written < large_data.size());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Non-blocking write completed: " << total_written 
              << " total bytes written in " << duration.count() << "ms\n";
}

void test_concurrent_read_write() {
    std::cout << "Testing concurrent read/write...\n";
    
    mx::Pipe pipe;
    std::vector<std::byte> write_data(2048);
    for (size_t i = 0; i < write_data.size(); ++i) {
        write_data[i] = static_cast<std::byte>('C');
    }
    
    std::thread reader([&pipe]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        std::vector<std::byte> read_buffer(2048);
        std::size_t bytes_read = pipe.read_data(read_buffer.data(), read_buffer.size());
        std::cout << "Reader: " << bytes_read << " bytes read\n";
    });
    
    std::thread writer([&pipe, &write_data]() {
        std::size_t bytes_written = pipe.write(write_data);
        std::cout << "Writer: " << bytes_written << " bytes written\n";
        pipe.close_write(); 
    });
    
    reader.join();
    writer.join();
}

int main() {
    std::cout << "mx::Pipe Write Test Program\n";
    
    try {
        test_blocking_write();
        std::cout << "\n";
        
        test_nonblocking_write();
        std::cout << "\n";
        
        test_concurrent_read_write();
        std::cout << "\n";
        
        std::cout << "All tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}