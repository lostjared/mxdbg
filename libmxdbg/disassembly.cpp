#include "mxdbg/disassembly.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <queue>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

    struct Block {
        std::uint64_t address{};
        std::string name;
        std::vector<std::string> lines;
        std::vector<std::uint64_t> jump_targets;
        bool falls_through = true;
    };

    std::string_view trim(std::string_view text) {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
            text.remove_prefix(1);
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
            text.remove_suffix(1);
        return text;
    }

    std::optional<std::pair<std::uint64_t, std::string>> parse_header(
        const std::string& line) {
        const std::string_view text = trim(line);
        const auto space = text.find_first_of(" \t");
        if (space == std::string_view::npos) return std::nullopt;
        const auto left = text.find('<', space);
        const auto right = text.rfind(">:");
        if (left == std::string_view::npos || right == std::string_view::npos ||
            right <= left || right + 2 != text.size())
            return std::nullopt;
        try {
            std::size_t parsed = 0;
            const auto address_text = std::string(text.substr(0, space));
            const auto address = std::stoull(address_text, &parsed, 16);
            if (parsed != address_text.size()) return std::nullopt;
            return std::pair{address, std::string(text.substr(left + 1, right - left - 1))};
        } catch (...) {
            return std::nullopt;
        }
    }

    bool is_hex_byte(std::string_view token) {
        return token.size() == 2 &&
               std::isxdigit(static_cast<unsigned char>(token[0])) &&
               std::isxdigit(static_cast<unsigned char>(token[1]));
    }

    struct Instruction {
        std::string mnemonic;
        std::string operand;
    };

    bool is_instruction_prefix(std::string_view token) {
        return token == "addr16" || token == "addr32" || token == "bnd" ||
               token == "cs" || token == "data16" || token == "data32" ||
               token == "ds" || token == "es" || token == "fs" || token == "gs" ||
               token == "lock" || token == "notrack" || token == "rep" ||
               token == "repe" || token == "repne" || token == "repnz" ||
               token == "repz" || token == "ss" || token.starts_with("rex");
    }

    std::optional<Instruction> parse_instruction(const std::string& line) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) return std::nullopt;
        std::istringstream fields(line.substr(colon + 1));
        std::string token;
        while (fields >> token) {
            if (is_hex_byte(token) || is_instruction_prefix(token)) continue;
            Instruction result;
            result.mnemonic = std::move(token);
            fields >> result.operand;
            return result;
        }
        return std::nullopt;
    }

    std::optional<std::uint64_t> parse_target(const std::string& operand) {
        std::string_view text = operand;
        if (!text.empty() && text.front() == '*') return std::nullopt;
        if (!text.empty() && text.front() == '$') text.remove_prefix(1);
        if (text.starts_with("0x")) text.remove_prefix(2);
        const auto length = text.find_first_not_of("0123456789abcdefABCDEF");
        text = text.substr(0, length);
        if (text.empty()) return std::nullopt;
        try {
            return std::stoull(std::string(text), nullptr, 16);
        } catch (...) {
            return std::nullopt;
        }
    }

    bool is_jump(std::string_view mnemonic) {
        return (!mnemonic.empty() && mnemonic.front() == 'j') ||
               mnemonic.starts_with("loop");
    }

    bool is_unconditional_jump(std::string_view mnemonic) {
        return mnemonic == "jmp" || mnemonic == "jmpq" || mnemonic == "ljmp";
    }

    bool ends_control_flow(std::string_view mnemonic) {
        return is_unconditional_jump(mnemonic) || mnemonic.starts_with("ret") ||
               mnemonic == "iret" || mnemonic == "iretq" || mnemonic == "ud2" ||
               mnemonic == "hlt";
    }

    bool is_padding(std::string_view mnemonic) {
        return mnemonic.starts_with("nop");
    }

} // namespace

namespace mx {

    std::string extract_function_disassembly(const std::string& disassembly,
                                             const std::string& function_name) {
        std::vector<Block> blocks;
        std::unordered_set<std::uint64_t> called_entries;
        std::istringstream input(disassembly);
        std::string line;
        Block* current = nullptr;

        while (std::getline(input, line)) {
            if (const auto header = parse_header(line)) {
                blocks.push_back(Block{header->first, header->second});
                current = &blocks.back();
                current->lines.push_back(line);
                continue;
            }
            if (!current) continue;
            if (!trim(line).empty()) current->lines.push_back(line);
            const auto instruction = parse_instruction(line);
            if (!instruction) continue;
            if (is_jump(instruction->mnemonic)) {
                if (const auto target = parse_target(instruction->operand))
                    current->jump_targets.push_back(*target);
            } else if (instruction->mnemonic == "call" || instruction->mnemonic == "callq") {
                if (const auto target = parse_target(instruction->operand))
                    called_entries.insert(*target);
            }
            // Alignment NOPs commonly follow RET before the next symbol.  They
            // do not make the previous function fall through into that symbol.
            if (!is_padding(instruction->mnemonic))
                current->falls_through = !ends_control_flow(instruction->mnemonic);
        }

        const auto start = std::find_if(blocks.begin(), blocks.end(), [&](const Block& block) {
            return block.name == function_name;
        });
        if (start == blocks.end()) return {};

        std::unordered_map<std::uint64_t, std::size_t> block_by_address;
        for (std::size_t index = 0; index < blocks.size(); ++index)
            block_by_address.emplace(blocks[index].address, index);

        const std::size_t start_index = static_cast<std::size_t>(start - blocks.begin());
        std::vector<bool> included(blocks.size());
        std::queue<std::size_t> pending;
        included[start_index] = true;
        pending.push(start_index);

        auto include_target = [&](std::uint64_t address) {
            const auto found = block_by_address.find(address);
            if (found == block_by_address.end()) return;
            const auto index = found->second;
            // A label reached by CALL is another function entry.  A JMP to it is
            // a tail call, not an internal basic-block edge.
            if (index != start_index && called_entries.contains(address)) return;
            if (!included[index]) {
                included[index] = true;
                pending.push(index);
            }
        };

        while (!pending.empty()) {
            const auto index = pending.front();
            pending.pop();
            for (const auto target : blocks[index].jump_targets) include_target(target);
            if (blocks[index].falls_through && index + 1 < blocks.size())
                include_target(blocks[index + 1].address);
        }

        std::ostringstream output;
        for (std::size_t index = 0; index < blocks.size(); ++index) {
            if (!included[index]) continue;
            if (output.tellp() > 0) output << '\n';
            for (const auto& block_line : blocks[index].lines) output << block_line << '\n';
        }
        return output.str();
    }

} // namespace mx
