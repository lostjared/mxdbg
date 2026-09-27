#ifndef MXDBG_DISASSEMBLY_HPP
#define MXDBG_DISASSEMBLY_HPP

#include <string>

namespace mx {

    // Extract every basic block reachable from a function entry.  Objdump prints
    // local assembly labels using the same header syntax as function symbols, so
    // simply stopping at the next header truncates functions containing labels.
    [[nodiscard]] std::string extract_function_disassembly(
        const std::string& disassembly, const std::string& function_name);

} // namespace mx

#endif
