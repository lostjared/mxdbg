#ifndef _PIPE_H__ 
#define _PIPE_H__

#include <cstdint>
#include <string>
#include <vector>
#include <memory>                                   
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
namespace mx {

    class Pipe {    
    public:
        Pipe();
        ~Pipe();
        Pipe(const Pipe&) = delete;
        Pipe& operator=(const Pipe&) = delete;
        Pipe(Pipe &&proc);
        Pipe &operator=(Pipe &&proc);

        void write(const std::string& data);
        std::string read();
        std::string read_nonblocking();
        bool is_open() const;
        bool is_readable() const;
        bool is_writable() const;
        int get_read_fd() const;
        int get_write_fd() const;
        void close_read();
        void close_write();
        void close();
    private:
        int m_read_fd;
        int m_write_fd;
    };

}


#endif