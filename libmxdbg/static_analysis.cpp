#include "mxdbg/static_analysis.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string_view>

namespace {
    struct Instruction {
        std::string mnemonic;
        std::vector<std::string> operands;
        std::string comment;
    };

    struct CallFact {
        std::string function;
        std::vector<std::pair<std::string, std::string>> arguments;
    };

    std::string trim(std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::vector<std::string> split_operands(const std::string& text) {
        std::vector<std::string> result;
        std::size_t start = 0;
        int parentheses = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '(') ++parentheses;
            else if (text[i] == ')') --parentheses;
            else if (text[i] == ',' && parentheses == 0) {
                result.push_back(trim(text.substr(start, i - start)));
                start = i + 1;
            }
        }
        if (start < text.size()) result.push_back(trim(text.substr(start)));
        return result;
    }

    std::optional<Instruction> parse_instruction(const std::string& line) {
        // objdump separates the address, bytes, and assembly using tabs. Using
        // the final tab also handles instructions whose byte encoding is long.
        const auto colon = line.find(':');
        if (colon == std::string::npos) return std::nullopt;
        const auto tab = line.find_last_of('\t');
        if (tab == std::string::npos || tab < colon) return std::nullopt;

        std::string assembly = trim(line.substr(tab + 1));
        if (assembly.empty()) return std::nullopt;

        Instruction instruction;
        if (const auto hash = assembly.find('#'); hash != std::string::npos) {
            instruction.comment = trim(assembly.substr(hash + 1));
            assembly = trim(assembly.substr(0, hash));
        }
        const auto space = assembly.find_first_of(" \t");
        instruction.mnemonic = assembly.substr(0, space);
        if (space != std::string::npos)
            instruction.operands = split_operands(assembly.substr(space + 1));
        return instruction;
    }

    std::string canonical_register(std::string value) {
        if (!value.empty() && value.front() == '%') value.erase(0, 1);
        static const std::unordered_map<std::string, std::string> aliases = {
            {"eax", "rax"}, {"ax", "rax"}, {"al", "rax"},
            {"edi", "rdi"}, {"di", "rdi"}, {"dil", "rdi"},
            {"esi", "rsi"}, {"si", "rsi"}, {"sil", "rsi"},
            {"edx", "rdx"}, {"dx", "rdx"}, {"dl", "rdx"},
            {"ecx", "rcx"}, {"cx", "rcx"}, {"cl", "rcx"},
            {"r8d", "r8"}, {"r8w", "r8"}, {"r8b", "r8"},
            {"r9d", "r9"}, {"r9w", "r9"}, {"r9b", "r9"}
        };
        if (const auto found = aliases.find(value); found != aliases.end())
            return found->second;
        return value;
    }

    bool is_register(const std::string& value) {
        return !value.empty() && value.front() == '%';
    }

    std::optional<std::int64_t> parse_integer(std::string value) {
        value = trim(value);
        if (!value.empty() && value.front() == '$') value.erase(0, 1);
        int base = 10;
        bool negative = false;
        if (!value.empty() && value.front() == '-') {
            negative = true;
            value.erase(0, 1);
        }
        if (value.starts_with("0x")) {
            base = 16;
            value.erase(0, 2);
        }
        std::int64_t parsed = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed, base);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            return std::nullopt;
        return negative ? -parsed : parsed;
    }

    std::string decimal_immediate(const std::string& value) {
        if (const auto parsed = parse_integer(value)) return std::to_string(*parsed);
        return value;
    }

    std::string stack_location(const std::string& operand) {
        static const std::regex stack_pattern(R"(^(-?(?:0x[0-9a-fA-F]+|[0-9]+))\(%rbp\)$)");
        std::smatch match;
        if (!std::regex_match(operand, match, stack_pattern)) return {};
        return "[rbp" + std::string(match[1]).replace(0, match[1].str().front() == '-' ? 1 : 0,
            match[1].str().front() == '-' ? "-" : "+") + "]";
    }

    std::string symbol_from_comment(const std::string& comment) {
        const auto left = comment.find('<');
        const auto right = comment.find('>', left == std::string::npos ? 0 : left + 1);
        if (left == std::string::npos || right == std::string::npos) return {};
        std::string symbol = comment.substr(left + 1, right - left - 1);
        if (const auto at = symbol.find('@'); at != std::string::npos) symbol.resize(at);
        if (symbol == "stderr" || symbol == "stdout" || symbol == "stdin") return symbol;
        return "constant pointer " + symbol;
    }

    std::string called_function(const Instruction& instruction) {
        if (!instruction.mnemonic.starts_with("call") || instruction.operands.empty()) return {};
        const std::string& target = instruction.operands.front();
        const auto left = target.find('<');
        const auto right = target.find('>', left == std::string::npos ? 0 : left + 1);
        if (left == std::string::npos || right == std::string::npos) return {};
        std::string name = target.substr(left + 1, right - left - 1);
        if (const auto plus = name.find('+'); plus != std::string::npos) name.resize(plus);
        if (const auto at = name.find('@'); at != std::string::npos) name.resize(at);
        return name;
    }

    std::string render_add(const std::string& base, std::int64_t amount) {
        if (base == "argv" && amount >= 0 && amount % 8 == 0)
            return "argv + " + std::to_string(amount);
        if (amount == 0) return base;
        return base + (amount > 0 ? " + " : " - ") + std::to_string(std::abs(amount));
    }

    std::string dereference(const std::string& value) {
        static const std::regex argv_offset(R"(^argv \+ ([0-9]+)$)");
        std::smatch match;
        if (std::regex_match(value, match, argv_offset)) {
            const auto offset = std::stoull(match[1]);
            if (offset % 8 == 0) return "argv[" + std::to_string(offset / 8) + "]";
        }
        if (value == "argv") return "argv[0]";
        return "[" + value + "]";
    }

    std::string normalize_memory(const std::string& operand) {
        if (const std::string stack = stack_location(operand); !stack.empty()) return stack;
        return operand;
    }
}

namespace mx {
    const std::unordered_map<std::string, FunctionSignature>& known_function_signatures() {
        static const std::unordered_map<std::string, FunctionSignature> signatures = {
            {"fread", {"fread", {"ptr", "size", "nmemb", "stream"}}},
            {"fwrite", {"fwrite", {"ptr", "size", "nmemb", "stream"}}},
            {"fopen", {"fopen", {"filename", "mode"}}},
            {"fprintf", {"fprintf", {"stream", "format"}}},
            {"printf", {"printf", {"format"}}},
            {"atoi", {"atoi", {"string"}}},
            {"putchar", {"putchar", {"character"}}},
            {"fflush", {"fflush", {"stream"}}},
            {"usleep", {"usleep", {"microseconds"}}},
            {"fclose", {"fclose", {"stream"}}},
            {"exit", {"exit", {"status"}}}
        };
        return signatures;
    }

    std::string derive_static_facts(const std::string& function_name,
                                    const std::string& disassembly) {
        std::vector<Instruction> instructions;
        std::istringstream input(disassembly);
        for (std::string line; std::getline(input, line);) {
            if (auto parsed = parse_instruction(line)) instructions.push_back(std::move(*parsed));
        }

        std::map<std::string, std::string> registers;
        std::map<std::string, std::string> locals;
        std::map<std::string, std::string> local_descriptions;
        std::vector<CallFact> calls;
        std::set<std::int64_t> argc_comparisons;
        const std::vector<std::string> argument_registers = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (function_name == "main") {
            registers["rdi"] = "argc";
            registers["rsi"] = "argv";
        }

        auto value_of = [&](const std::string& operand, const std::string& comment) {
            if (operand.empty()) return std::string{};
            if (operand.front() == '$') return decimal_immediate(operand);
            if (is_register(operand)) {
                const std::string reg = canonical_register(operand);
                const auto found = registers.find(reg);
                return found == registers.end() ? std::string{} : found->second;
            }
            if (const std::string stack = stack_location(operand); !stack.empty()) {
                const auto found = locals.find(stack);
                return found == locals.end() ? stack : found->second;
            }
            if (operand.find("(%rip)") != std::string::npos) {
                const std::string symbol = symbol_from_comment(comment);
                return symbol.empty() ? normalize_memory(operand) : symbol;
            }
            static const std::regex indirect(R"(^\((%[a-zA-Z0-9]+)\)$)");
            std::smatch match;
            if (std::regex_match(operand, match, indirect)) {
                const std::string reg = canonical_register(match[1]);
                const auto found = registers.find(reg);
                return found == registers.end() ? normalize_memory(operand) : dereference(found->second);
            }
            return normalize_memory(operand);
        };

        for (const auto& instruction : instructions) {
            const std::string function = called_function(instruction);
            if (!function.empty()) {
                std::optional<std::string> written_local;
                if (const auto signature = known_function_signatures().find(function);
                    signature != known_function_signatures().end()) {
                    CallFact fact{function, {}};
                    for (std::size_t index = 0; index < argument_registers.size(); ++index) {
                        const auto value = registers.find(argument_registers[index]);
                        if (value == registers.end()) continue;
                        std::string name = index < signature->second.arguments.size()
                            ? signature->second.arguments[index]
                            : "argument " + std::to_string(index + 1);
                        fact.arguments.emplace_back(std::move(name), value->second);
                    }
                    if (function == "fread") {
                        const auto pointer = std::find_if(fact.arguments.begin(), fact.arguments.end(),
                            [](const auto& argument) { return argument.first == "ptr"; });
                        if (pointer != fact.arguments.end() && pointer->second.starts_with("[rbp")) {
                            written_local = pointer->second;
                            const auto size = std::find_if(fact.arguments.begin(), fact.arguments.end(),
                                [](const auto& argument) { return argument.first == "size"; });
                            const auto count = std::find_if(fact.arguments.begin(), fact.arguments.end(),
                                [](const auto& argument) { return argument.first == "nmemb"; });
                            local_descriptions[pointer->second] =
                                size != fact.arguments.end() && count != fact.arguments.end() &&
                                size->second == "1" && count->second == "1"
                                    ? "one-byte buffer"
                                    : "buffer written by fread";
                        }
                    }
                    calls.push_back(std::move(fact));
                }
                if (written_local) locals.erase(*written_local);
                registers.clear();
                registers["rax"] = "return value of " + function;
                continue;
            }

            if (instruction.mnemonic.starts_with("mov") && instruction.operands.size() == 2) {
                const std::string value = value_of(instruction.operands[0], instruction.comment);
                const std::string& destination = instruction.operands[1];
                if (is_register(destination)) {
                    const std::string reg = canonical_register(destination);
                    if (value.empty()) registers.erase(reg);
                    else registers[reg] = value;
                } else if (const std::string stack = stack_location(destination); !stack.empty()) {
                    if (!value.empty()) locals[stack] = value;
                }
                continue;
            }

            if (instruction.mnemonic.starts_with("lea") && instruction.operands.size() == 2 &&
                is_register(instruction.operands[1])) {
                std::string value;
                if (instruction.operands[0].find("(%rip)") != std::string::npos)
                    value = symbol_from_comment(instruction.comment);
                else if (const std::string stack = stack_location(instruction.operands[0]); !stack.empty())
                    value = stack;
                else
                    value = value_of(instruction.operands[0], instruction.comment);
                if (!value.empty()) registers[canonical_register(instruction.operands[1])] = value;
                continue;
            }

            if (instruction.mnemonic.starts_with("add") && instruction.operands.size() == 2 &&
                is_register(instruction.operands[1])) {
                const std::string reg = canonical_register(instruction.operands[1]);
                if (const auto amount = parse_integer(instruction.operands[0]); amount && registers.contains(reg))
                    registers[reg] = render_add(registers[reg], *amount);
                else
                    registers.erase(reg);
                continue;
            }

            if (instruction.mnemonic.starts_with("imul") && instruction.operands.size() >= 2) {
                const auto amount = parse_integer(instruction.operands[0]);
                const std::string destination = canonical_register(instruction.operands.back());
                const std::string source_register = canonical_register(instruction.operands[instruction.operands.size() - 2]);
                if (amount && registers.contains(source_register))
                    registers[destination] = registers[source_register] + " * " + std::to_string(*amount);
                else
                    registers.erase(destination);
                continue;
            }

            if (instruction.mnemonic.starts_with("cmp") && instruction.operands.size() == 2) {
                const auto immediate = parse_integer(instruction.operands[0]);
                const std::string compared = value_of(instruction.operands[1], instruction.comment);
                if (immediate && compared == "argc") argc_comparisons.insert(*immediate);
                continue;
            }

            // Register values are not carried across control-flow boundaries.
            if (instruction.mnemonic.starts_with('j') || instruction.mnemonic == "ret" ||
                instruction.mnemonic == "leave") {
                registers.clear();
            }
        }

        std::map<std::string, std::string> reported_locals = local_descriptions;
        for (const auto& [location, value] : locals) {
            if (value.starts_with("return value of ")) reported_locals[location] = value;
        }

        if (calls.empty() && reported_locals.empty() && argc_comparisons.empty()) return {};

        std::ostringstream output;
        output << "DERIVED STATIC FACTS\n";
        if (!argc_comparisons.empty()) {
            output << "\nargc:\n";
            for (const auto value : argc_comparisons) {
                output << "    compared against " << value << "\n";
                if (value > 0) output << "    therefore that check corresponds to "
                                      << value - 1 << " user-supplied argument(s)\n";
            }
        }
        if (!reported_locals.empty()) {
            output << "\nlocals:\n";
            for (const auto& [location, value] : reported_locals)
                output << "    " << location << " = " << value << "\n";
        }
        if (!calls.empty()) {
            output << "\ncalls:\n";
            for (const auto& call : calls) {
                output << "\n    " << call.function << ":\n";
                const auto& signature = known_function_signatures().at(call.function);
                output << "        prototype: " << call.function << "(";
                for (std::size_t i = 0; i < signature.arguments.size(); ++i) {
                    if (i != 0) output << ", ";
                    output << signature.arguments[i];
                }
                output << ")\n";
                for (const auto& [name, value] : call.arguments) {
                    const auto named = std::find(signature.arguments.begin(), signature.arguments.end(), name);
                    if (named != signature.arguments.end()) {
                        const auto index = static_cast<std::size_t>(named - signature.arguments.begin());
                        output << "        " << argument_registers[index] << " = " << value << "\n";
                    } else if (name.starts_with("argument ")) {
                        if (const auto index = parse_integer(name.substr(9)); index && *index > 0 &&
                            static_cast<std::size_t>(*index) <= argument_registers.size())
                            output << "        " << argument_registers[*index - 1] << " = " << value << "\n";
                    }
                }
                for (const auto& [name, value] : call.arguments)
                    output << "        " << name << " = " << value << "\n";
                output << "        interpreted call: " << call.function << "(";
                for (std::size_t i = 0; i < signature.arguments.size(); ++i) {
                    if (i != 0) output << ", ";
                    const auto found = std::find_if(call.arguments.begin(), call.arguments.end(),
                        [&](const auto& argument) { return argument.first == signature.arguments[i]; });
                    output << (found == call.arguments.end() ? "unknown" : found->second);
                }
                output << ")\n";
            }
        }
        return output.str();
    }
}
