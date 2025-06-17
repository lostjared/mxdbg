#ifndef _PIPE_H__ 
#define _PIPE_H__

#include<cstdint>
#include<string>
#include<vector>
#include<memory>                                   
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<cstddef>


namespace mx {

    class Pipe {    
    public:
        Pipe(bool on_close = true);
        ~Pipe();
        Pipe(const Pipe&) = delete;
        Pipe& operator=(const Pipe&) = delete;
        Pipe(Pipe &&proc);
        Pipe &operator=(Pipe &&proc);

        std::size_t write(const std::string& data);
        std::size_t write(const std::byte *data, std::size_t size);
        std::size_t write(const std::vector<std::byte> &data);

        std::string read();
        std::vector<std::byte> read_bytes();
        std::size_t read_data(std::byte *data, std::size_t size);
        std::size_t read_data_nonblocking(std::byte *data, std::size_t size);
        std::vector<std::byte> read_vec_nonblocking(std::size_t size);
        std::string read_nonblocking();
        std::vector<std::byte> read_bytes_nonblocking();
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