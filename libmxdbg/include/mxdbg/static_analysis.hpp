#ifndef MXDBG_STATIC_ANALYSIS_HPP
#define MXDBG_STATIC_ANALYSIS_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace mx {
    struct FunctionSignature {
        std::string name;
        std::vector<std::string> arguments;
    };

    [[nodiscard]] const std::unordered_map<std::string, FunctionSignature>&
    known_function_signatures();

    // Derive conservative, human-readable facts from GNU objdump AT&T syntax.
    // An empty string means that no supported facts were found.
    [[nodiscard]] std::string derive_static_facts(
        const std::string& function_name,
        const std::string& disassembly);
}

#endif
