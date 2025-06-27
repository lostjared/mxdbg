#ifndef _EXCEPTION_H_1
#define _EXCEPTION_H_1

#include<stdexcept>
#include<string>
#include<errno.h>
#include<cstring>
#include<cstdint>

namespace mx {

    std::string format_hex64(uint64_t value);
    std::string format_hex32(uint32_t value);
    std::string format_hex16(uint16_t value);
    std::string format_hex8(uint8_t value);
    std::string format_signal(uint32_t sig);

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
}

#endif