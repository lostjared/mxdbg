/*
    MXDBG - Debugger with AI
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef ___DEBUGGER__H__
#define ___DEBUGGER__H__

#include "mxdbg/process.hpp"
#include "mxdbg/debug_context.hpp"
#include "mxdbg/ai_config.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mx2-ollama.hpp>
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace mx {

    [[nodiscard]] std::vector<std::string> split_command(const std::string &cmd);
    [[nodiscard]] std::string join(size_t start, size_t stop, std::vector<std::string> &tokens, const std::string &delimiter);

    struct MemoryRegion {
        uint64_t start{};
        uint64_t end{};
        std::string permissions;
        std::string pathname;
        [[nodiscard]] bool is_readable() const { return permissions[0] == 'r'; }
        [[nodiscard]] bool is_writable() const { return permissions[1] == 'w'; }
        [[nodiscard]] bool is_executable() const { return permissions[2] == 'x'; }
    };

    struct ThreadInfo {
        pid_t tid{};
        std::string state;
        uint64_t rip{};
        std::string name;
    };

    enum class StepOutResult {
        Completed,
        Signal,
        ProcessExited,
        Error
    };

    class Debugger {
      public:
        explicit Debugger(bool ai = true);
        ~Debugger();

        bool attach(pid_t pid);
        bool launch(const std::filesystem::path &exe, std::string_view args);
        void dump_file(const std::filesystem::path &file);
        void print_address() const;
        void continue_execution();
        void wait_for_stop();
        void step();
        void step_n(int count);

        [[nodiscard]] pid_t get_pid() const;
        [[nodiscard]] bool is_running() const;

        bool command(const std::string &cmd);
        [[nodiscard]] uint64_t expression(const std::string &text);

        void setup_history();
        void save_history();
        void detach();

        [[nodiscard]] uint64_t get_base_address() const;
        bool setfunction_breakpoint(const std::string &function_name);
        [[nodiscard]] uint64_t calculate_variable_address(const std::string &r, uint64_t value);
        void break_if(uint64_t location, const std::string &e);
        void step_over();
        [[nodiscard]] StepOutResult step_out();
        void run_until(uint64_t address);

      private:
        std::string print_current_instruction();
        [[nodiscard]] std::string current_instruction_text(uint64_t rip) const;
        std::unique_ptr<Process> process;
        std::string_view args;
        pid_t p_id = -1;
        std::string history_filename;
        std::string program_name;
        std::string_view args_string;
        std::unique_ptr<mx::ObjectRequest> request;
        std::string ai_configuration_error;
        std::uint64_t captured_stop_sequence = 0;
        [[nodiscard]] std::string obj_dump() const;
        std::string user_mode = "programmer";
        DebugContext context;

        [[nodiscard]] std::string functionText(const std::string &text);
        void print_backtrace() const;
        void wait_for_process_stop();
        void wait_for_single_step();
        void capture_crash_context();
        [[nodiscard]] std::string render_current_debugger_state() const;
        [[nodiscard]] std::vector<uint64_t> get_stack_frames() const;
        struct CallerFrame {
            enum class Source { StackPointer, FramePointer, StackScan } source;
            uint64_t return_address{};
            uint64_t next_frame_pointer{};
            std::size_t stack_offset{};
        };
        [[nodiscard]] std::optional<CallerFrame> find_caller_frame(
            uint64_t rip, uint64_t rbp, uint64_t rsp) const;
        [[nodiscard]] bool is_valid_return_address(
            uint64_t address, uint64_t current_rip,
            const std::optional<std::pair<uint64_t, uint64_t>>& current_function) const;
        [[nodiscard]] bool instruction_before_is_call(uint64_t address) const;
        [[nodiscard]] std::optional<std::pair<uint64_t, uint64_t>>
            function_range_containing(uint64_t address) const;
        [[nodiscard]] std::string resolve_symbol(uint64_t address) const;
        [[nodiscard]] bool is_at_function_entry() const;
        [[nodiscard]] bool is_valid_code_address(uint64_t address) const;
        void analyze_current_frame() const;
        void print_memory_maps() const;
        void search_memory_for_int32(int32_t value);
        void search_memory_for_int64(int64_t value);
        void search_memory_for_string(const std::string &pattern);
        void search_memory_for_bytes(std::span<const std::string> byte_tokens);
        void search_memory_for_pattern(const std::string &pattern);
        [[nodiscard]] std::vector<MemoryRegion> get_searchable_memory_regions();
        [[nodiscard]] bool match_pattern(std::span<const uint8_t> data, const std::string &pattern, size_t offset);
        [[nodiscard]] std::vector<size_t> find_in_memory(std::span<const uint8_t> haystack, std::span<const uint8_t> needle);
        [[nodiscard]] bool parse_pattern(const std::string &pattern, std::vector<std::pair<uint8_t, bool>> &parsed_pattern);
        [[nodiscard]] std::vector<size_t> find_pattern_in_memory(std::span<const uint8_t> memory, std::span<const std::pair<uint8_t, bool>> pattern);
        void list_threads();
        void switch_thread(pid_t id);
    };

} // namespace mx

#endif
