#include "mxdbg/exception.hpp"
#include<iostream>
#include<cstdint>
#include<iomanip>

namespace mx {
    std::string format_hex64(uint64_t value) {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(16) << std::hex << value;
        return oss.str();
    }

    std::string format_hex32(uint32_t value) {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
        return oss.str();
    }

    std::string format_hex16(uint16_t value) {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(4) << std::hex << value;
        return oss.str();
    }
    
    std::string format_hex8(uint8_t value) {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(3) << std::hex << value;
        return oss.str();
    }
}