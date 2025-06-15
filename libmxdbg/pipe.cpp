#include"mxdbg/pipe.hpp"
#include<unistd.h>
#include<errno.h>
#include<err.h>
#include<cstring>

namespace mx {
    
    mx::Pipe::Pipe() {
        int fds[2];
        if (pipe(fds) == -1) {
            throw std::runtime_error("Failed to create pipe: " + std::string(strerror(errno)));
        }
        m_read_fd = fds[0];
        m_write_fd = fds[1];           
    }

    Pipe::Pipe(Pipe &&proc) : m_read_fd(proc.m_read_fd), m_write_fd(proc.m_write_fd) {
        proc.m_read_fd = -1;
        proc.m_write_fd = -1;
    }

    Pipe &Pipe::operator=(Pipe &&proc) {
        if (this != &proc) {
            close();
            m_read_fd = proc.m_read_fd;
            m_write_fd = proc.m_write_fd;
            proc.m_read_fd = -1;
            proc.m_write_fd = -1;
        }
        return *this;
    }

    mx::Pipe::~Pipe() {
        close();
    }

    void Pipe::write(const std::string& data) {
        if (m_write_fd == -1) {
            throw std::runtime_error("Pipe is closed for writing");
        }
        
        size_t total_written = 0;
        const char* ptr = data.c_str();
        
        while (total_written < data.size()) {
            ssize_t bytes_written = ::write(m_write_fd, ptr + total_written, data.size() - total_written);
            if (bytes_written == -1) {
                if (errno == EINTR) {
                    continue; 
                }
                throw std::runtime_error("Failed to write to pipe: " + std::string(strerror(errno)));
            }
            total_written += bytes_written;
        }
    }

    std::string Pipe::read() {
        if (m_read_fd == -1) {
            throw std::runtime_error("Pipe is closed for reading");
        }
        char buffer[1024];
        ssize_t bytes_read = ::read(m_read_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                return ""; 
            }
            throw std::runtime_error("Failed to read from pipe: " + std::string(strerror(errno)));
        }
        
        if (bytes_read == 0) {
            return ""; 
        }

        return std::string(buffer, bytes_read);
    }
    
    void Pipe::close() {
        if (m_read_fd != -1) {
            ::close(m_read_fd);
            m_read_fd = -1;
        }
        if (m_write_fd != -1) {
            ::close(m_write_fd);
            m_write_fd = -1;
        }
    }

    void Pipe::close_read() {
        if (m_read_fd != -1) {
            ::close(m_read_fd);
            m_read_fd = -1;
        }
    }

    void Pipe::close_write() {
        if (m_write_fd != -1) {
            ::close(m_write_fd);
            m_write_fd = -1;
        }
    }

    bool Pipe::is_readable() const {
        return m_read_fd != -1;
    }

    bool Pipe::is_writable() const {
        return m_write_fd != -1;
    }

    int Pipe::get_read_fd() const {
        return m_read_fd;
    }

    int Pipe::get_write_fd() const {
        return m_write_fd;
    }

    std::string Pipe::read_nonblocking() {
        if (m_read_fd == -1) {
            throw std::runtime_error("Pipe is closed for reading");
        }
        
        // Get current flags
        int flags = fcntl(m_read_fd, F_GETFL);
        if (flags == -1) {
            throw std::runtime_error("Failed to get file flags: " + std::string(strerror(errno)));
        }
        
        // Set non-blocking mode
        if (fcntl(m_read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        }
        
        char buffer[1024];
        ssize_t bytes_read = ::read(m_read_fd, buffer, sizeof(buffer) - 1);
        
        // Restore original flags
        fcntl(m_read_fd, F_SETFL, flags);
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return ""; // No data available
            }
            throw std::runtime_error("Failed to read from pipe: " + std::string(strerror(errno)));
        }
        
        if (bytes_read == 0) {
            return ""; // EOF
        }
        
        return std::string(buffer, bytes_read);
    }
}
