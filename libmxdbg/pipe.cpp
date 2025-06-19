#include"mxdbg/pipe.hpp"
#include"mxdbg/exception.hpp"
#include<unistd.h>
#include<errno.h>
#include<err.h>
#include<cstring>

namespace mx {
    
    mx::Pipe::Pipe(bool on_close) {
        int fds[2];
        if (pipe2(fds, on_close ? O_CLOEXEC :  0) == -1) {
            throw mx::Exception::error("Failed to create pipe");
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

    std::size_t Pipe::write(const std::byte *data, std::size_t size) {
        if (m_write_fd == -1) {
            throw mx::Exception("Pipe is closed for writing");
        }
        size_t total_written = 0;
        const char* ptr = reinterpret_cast<const char*>(data);
        while (total_written < size) {
            ssize_t bytes_written = ::write(m_write_fd, ptr + total_written, size - total_written);
            if (bytes_written == -1) {
                if (errno == EINTR) {
                    continue; 
                }
                throw mx::Exception::error("Failed to write to pipe");
            }
            total_written += bytes_written;
        }
        return total_written;
    }

    std::size_t Pipe::write(const std::vector<std::byte> &data) {
        return write(data.data(), data.size()); 
    }

    std::size_t Pipe::write(const std::string& data) {
        if (m_write_fd == -1) {
            throw mx::Exception("Pipe is closed for writing");
        }
        
        std::size_t total_written = 0;
        const char* ptr = data.c_str();
        
        while (total_written < data.size()) {
            ssize_t bytes_written = ::write(m_write_fd, ptr + total_written, data.size() - total_written);
            if (bytes_written == -1) {
                if (errno == EINTR) {
                    continue; 
                }
                throw mx::Exception::error("Failed to write to pipe");
            }
            total_written += bytes_written;
        }
        return total_written;
    }

    std::size_t Pipe::write_nonblocking(const std::byte *data, std::size_t size) {
        if (m_write_fd == -1) {
            throw mx::Exception("Pipe is closed for writing");
        }
        
        int flags = fcntl(m_write_fd, F_GETFL);
        if (flags == -1) {
            throw mx::Exception::error("Failed to get file flags");
        }
        
        if (fcntl(m_write_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw mx::Exception::error("Failed to set non-blocking mode");
        }
        
        size_t total_written = 0;
        const char* ptr = reinterpret_cast<const char*>(data);
        
        while (total_written < size) {
            ssize_t bytes_written = ::write(m_write_fd, ptr + total_written, size - total_written);
            if (bytes_written == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; 
                }
                
                fcntl(m_write_fd, F_SETFL, flags);
                throw mx::Exception::error("Failed to write to pipe");
            }
            total_written += bytes_written;
        }
        
        fcntl(m_write_fd, F_SETFL, flags);
        return total_written;
    }

    std::size_t Pipe::write_nonblocking(const std::vector<std::byte> &data) {
        return write_nonblocking(data.data(), data.size());
    }

    std::size_t Pipe::write_nonblocking(const std::string& data) {
        return write_nonblocking(reinterpret_cast<const std::byte*>(data.data()), data.size());
    }

    bool Pipe::is_open() const {
        return m_read_fd != -1 || m_write_fd != -1;
    }           
    std::string Pipe::read() {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        char buffer[1024];
        ssize_t bytes_read = ::read(m_read_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                return ""; 
            }
            throw mx::Exception::error("Failed to read from pipe");
        }
        
        if (bytes_read == 0) {
            return ""; 
        }

        return std::string(buffer, bytes_read);
    }

    std::vector<std::byte> Pipe::read_bytes() {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        
        std::vector<std::byte> buffer(1024);
        ssize_t bytes_read = ::read(m_read_fd, buffer.data(), buffer.size());
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                return {}; 
            }       
            throw mx::Exception::error("Failed to read from pipe");
        }
        
        if (bytes_read == 0) {
            return {}; 
        }

        buffer.resize(bytes_read);
        return buffer;
    }

    std::vector<std::byte> Pipe::read_bytes_nonblocking() {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        
        int flags = fcntl(m_read_fd, F_GETFL);
        if (flags == -1) {
            throw mx::Exception::error("Failed to get file flags");
        }
        
        if (fcntl(m_read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw mx::Exception::error("Failed to set non-blocking mode");
        }
        
        std::vector<std::byte> buffer(1024);
        ssize_t bytes_read = ::read(m_read_fd, buffer.data(), buffer.size());        
        fcntl(m_read_fd, F_SETFL, flags);
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {}; 
            }
            throw mx::Exception::error("Failed to read from pipe");
        }
        
        if (bytes_read == 0) {
            return {}; 
        }

        buffer.resize(bytes_read);
        return buffer;
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
            throw mx::Exception("Pipe is closed for reading");
        }
            
        int flags = fcntl(m_read_fd, F_GETFL);
        if (flags == -1) {
            throw mx::Exception::error("Failed to get file flags");
        }
        
        if (fcntl(m_read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw mx::Exception::error("Failed to set non-blocking mode");
        }
        
        char buffer[1024];
        ssize_t bytes_read = ::read(m_read_fd, buffer, sizeof(buffer) - 1);
        fcntl(m_read_fd, F_SETFL, flags);
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return ""; 
            }
            throw mx::Exception::error("Failed to read from pipe");
        }
        
        if (bytes_read == 0) {
            return ""; 
        }
        return std::string(buffer, bytes_read);
    }

    std::size_t Pipe::read_data(std::byte *data, std::size_t size) {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        size_t total_read = 0;
        while (total_read < size) {
            ssize_t bytes_read = ::read(m_read_fd, data + total_read, size - total_read);
            if (bytes_read == -1) {
                if (errno == EINTR) {
                    continue; 
                }
                throw mx::Exception::error("Failed to read from pipe");
            }
            if (bytes_read == 0) {
                break; 
            }
            total_read += bytes_read;
        }
        return total_read;
    }

    std::size_t Pipe::read_data_nonblocking(std::byte *data, std::size_t size) {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        int flags = fcntl(m_read_fd, F_GETFL);
        if (flags == -1) {
            throw mx::Exception::error("Failed to get file flags");
        }
        
        if (fcntl(m_read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw mx::Exception::error("Failed to set non-blocking mode");
        }
        
        size_t total_read = 0;
        while (total_read < size) {
            ssize_t bytes_read = ::read(m_read_fd, data + total_read, size - total_read);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; 
                }
                throw mx::Exception::error("Failed to read from pipe");
            }
            if (bytes_read == 0) {
                break; 
            }
            total_read += bytes_read;
        }
        
        fcntl(m_read_fd, F_SETFL, flags);
        return total_read;
    }

    std::vector<std::byte> Pipe::read_vec_nonblocking(std::size_t size) {
        if (m_read_fd == -1) {
            throw mx::Exception("Pipe is closed for reading");
        }
        
        int flags = fcntl(m_read_fd, F_GETFL);
        if (flags == -1) {
            throw mx::Exception::error("Failed to get file flags");
        }
        
        if (fcntl(m_read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw mx::Exception::error("Failed to set non-blocking mode");
        }
        
        std::vector<std::byte> buffer(size);
        ssize_t bytes_read = ::read(m_read_fd, buffer.data(), size);
        
        fcntl(m_read_fd, F_SETFL, flags);
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {}; 
            }
            throw mx::Exception::error("Failed to read from pipe");
        }
        
        if (bytes_read == 0) {
            return {}; 
        }

        buffer.resize(bytes_read);
        return buffer;  
    } 
}           
