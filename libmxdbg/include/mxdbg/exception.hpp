/*
    MXDBG - Debugger with AI
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef _EXCEPTION_H_1
#define _EXCEPTION_H_1

#include <cstdint>
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <termios.h>
#include <unistd.h>
namespace mx {

    [[nodiscard]] std::string format_hex64(uint64_t value);
    [[nodiscard]] std::string format_hex32(uint32_t value);
    [[nodiscard]] std::string format_hex16(uint16_t value);
    [[nodiscard]] std::string format_hex8(uint8_t value);
    [[nodiscard]] std::string format_signal(uint32_t sig);
    [[nodiscard]] std::string format_hex_no_prefix(uint64_t value);

    class Exception : public std::runtime_error {
      public:
        explicit Exception(const std::string &message)
            : std::runtime_error(message) {}

        explicit Exception(const char *message)
            : std::runtime_error(message) {}

        [[nodiscard]] static Exception error(const std::string &context) {
            return Exception(context + ": " + std::strerror(errno));
        }
    };

    namespace Color {
        constexpr std::string_view RESET = "\033[0m";
        constexpr std::string_view BOLD = "\033[1m";
        constexpr std::string_view DIM = "\033[2m";
        constexpr std::string_view BLACK = "\033[30m";
        constexpr std::string_view RED = "\033[31m";
        constexpr std::string_view GREEN = "\033[32m";
        constexpr std::string_view YELLOW = "\033[33m";
        constexpr std::string_view BLUE = "\033[34m";
        constexpr std::string_view MAGENTA = "\033[35m";
        constexpr std::string_view CYAN = "\033[36m";
        constexpr std::string_view WHITE = "\033[37m";
        constexpr std::string_view BRIGHT_RED = "\033[91m";
        constexpr std::string_view BRIGHT_GREEN = "\033[92m";
        constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
        constexpr std::string_view BRIGHT_BLUE = "\033[94m";
        constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
        constexpr std::string_view BRIGHT_CYAN = "\033[96m";
        constexpr std::string_view BG_RED = "\033[41m";
        constexpr std::string_view BG_GREEN = "\033[42m";
        constexpr std::string_view BG_YELLOW = "\033[43m";
    } // namespace Color
    inline bool terminal_supports_color() {
        return isatty(STDOUT_FILENO) && getenv("TERM") != nullptr;
    }
    inline bool color_ = terminal_supports_color();
} // namespace mx

#endif