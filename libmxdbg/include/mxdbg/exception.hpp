/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef _EXCEPTION_H_1
#define _EXCEPTION_H_1

#include<stdexcept>
#include<string>
#include<errno.h>
#include<cstring>
#include<cstdint>
#include<termios.h>
#include<unistd.h>
namespace mx {

    std::string format_hex64(uint64_t value);
    std::string format_hex32(uint32_t value);
    std::string format_hex16(uint16_t value);
    std::string format_hex8(uint8_t value);
    std::string format_signal(uint32_t sig);
    std::string format_hex_no_prefix(uint64_t value);

    class Exception: public std::runtime_error {
    public:
        explicit Exception(const std::string& message)
            : std::runtime_error(message) {}

        explicit Exception(const char* message)
            : std::runtime_error(message) {}

        static Exception error(const std::string& context) {
            return Exception(context + ": " + std::strerror(errno));
        }
    };

    namespace Color {
        const std::string RESET = "\033[0m";
        const std::string BOLD = "\033[1m";
        const std::string DIM = "\033[2m";
        const std::string BLACK = "\033[30m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string WHITE = "\033[37m";
        const std::string BRIGHT_RED = "\033[91m";
        const std::string BRIGHT_GREEN = "\033[92m";
        const std::string BRIGHT_YELLOW = "\033[93m";
        const std::string BRIGHT_BLUE = "\033[94m";
        const std::string BRIGHT_MAGENTA = "\033[95m";
        const std::string BRIGHT_CYAN = "\033[96m";        
        const std::string BG_RED = "\033[41m";
        const std::string BG_GREEN = "\033[42m";
        const std::string BG_YELLOW = "\033[43m";
    }   
    inline bool terminal_supports_color() {
        return isatty(STDOUT_FILENO) && getenv("TERM") != nullptr;
    }
    inline bool color_ = terminal_supports_color();
}

#endif