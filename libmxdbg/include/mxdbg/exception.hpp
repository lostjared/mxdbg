#ifndef _EXCEPTION_H_1
#define _EXCEPTION_H_1

#include <stdexcept>
#include <string>
#include<errno.h>
#include<cstring>

namespace mx {

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