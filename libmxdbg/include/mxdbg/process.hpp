/*
    MXDBG - Debugger with AI
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef MXDBG_PROCESS_HPP
#define MXDBG_PROCESS_HPP

#include <filesystem>
#include <cstdint>
#include <linux/elf.h>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <unistd.h>
#include <vector>

namespace mx {

    enum class StopKind {
        None,
        Exited,
        TerminatedBySignal,
        SignalStop,
        Breakpoint,
        Watchpoint,
        Trap
    };

    struct StopInfo {
        StopKind kind = StopKind::None;
        pid_t thread_id = -1;
        int signal = 0;
        int exit_code = 0;
        std::uint64_t sequence = 0;
    };

    struct SignalInfo {
        int code = 0;
        std::uint64_t fault_address = 0;
    };

    enum class WatchType {
        READ = 1,
        WRITE = 2,
        ACCESS = 3
    };

    struct Watchpoint {
        uint64_t address{};
        size_t size{};
        WatchType type{};
        std::string description;
        std::vector<uint8_t> instruction_bytes;
        std::string disassembly;
    };

    struct ConditionalBreakpoint {
        uint64_t address{};
        uint8_t original_byte{};
        std::string condition;
        bool is_conditional{false};

        ConditionalBreakpoint() = default;
        ConditionalBreakpoint(uint64_t addr, uint8_t orig, const std::string &cond = "")
            : address(addr), original_byte(orig), condition(cond), is_conditional(!cond.empty()) {}
    };

    class Process {
      public:
        Process(const Process &) = delete;
        ~Process() = default;
        Process &operator=(const Process &) = delete;
        Process(Process &&);
        Process &operator=(Process &&);
        [[nodiscard]] static std::unique_ptr<Process> launch(const std::filesystem::path &exe, const std::vector<std::string> &args = {});
        [[nodiscard]] static std::unique_ptr<Process> attach(pid_t pid);
        void continue_execution();
        void wait_for_stop();
        [[nodiscard]] pid_t get_pid() const { return m_pid; }
        [[nodiscard]] bool is_running() const;
        void detach();
        [[nodiscard]] int get_exit_status();
        [[nodiscard]] std::string proc_info() const;
        [[nodiscard]] std::string reg_info() const;
        void single_step();
        void wait_for_single_step();
        [[nodiscard]] uint64_t get_register(const std::string &reg_name) const;
        [[nodiscard]] uint32_t get_register_32(const std::string &reg_name) const;
        [[nodiscard]] uint16_t get_register_16(const std::string &reg_name) const;
        [[nodiscard]] uint8_t get_register_8(const std::string &reg_name) const;
        [[nodiscard]] std::vector<std::string> get_all_registers() const;
        void print_all_registers() const;
        void set_register(const std::string &reg_name, uint64_t value);
        void set_register_32(const std::string &reg_name, uint32_t value);
        void set_register_16(const std::string &reg_name, uint16_t value);
        void set_register_8(const std::string &reg_name, uint8_t value);
        [[nodiscard]] std::vector<uint8_t> read_memory(uint64_t address, size_t size) const;
        void write_memory(uint64_t address, std::span<const uint8_t> data);
        [[nodiscard]] uint64_t get_pc() const;
        [[nodiscard]] std::string disassemble_instruction(uint64_t address) const;
        [[nodiscard]] std::string get_current_instruction() const;
        void set_breakpoint(uint64_t address);
        bool remove_breakpoint(uint64_t address);
        bool remove_breakpoint_by_index(size_t index);
        [[nodiscard]] bool has_breakpoint(uint64_t address) const;
        [[nodiscard]] std::vector<std::pair<uint64_t, uint64_t>> get_breakpoints() const;
        [[nodiscard]] std::vector<std::pair<size_t, uint64_t>> get_breakpoints_with_index() const;
        [[nodiscard]] uint64_t get_breakpoint_address_by_index(size_t index) const;
        [[nodiscard]] size_t get_breakpoint_index_by_address(uint64_t address) const;
        [[nodiscard]] uint8_t get_original_instruction(uint64_t address) const;
        void handle_breakpoint_continue(uint64_t address);
        bool set_watchpoint(uint64_t address, size_t size, WatchType type);
        bool remove_watchpoint(uint64_t address);
        [[nodiscard]] std::vector<Watchpoint> get_watchpoints() const;
        [[nodiscard]] bool has_watchpoint_at(uint64_t address) const;
        [[nodiscard]] std::string disassemble_instruction(uint64_t address, const std::vector<uint8_t> &bytes) const;
        void switch_to_thread(pid_t id);
        [[nodiscard]] pid_t get_current_thread() const;
        [[nodiscard]] const StopInfo& get_last_stop() const { return last_stop_; }
        [[nodiscard]] std::optional<SignalInfo> get_signal_info() const;
        [[nodiscard]] std::vector<pid_t> get_thread_ids() const;
        [[nodiscard]] bool is_valid_thread(pid_t tid) const;
        void handle_thread_breakpoint_continue(uint64_t address);
        void break_if(uint64_t address, const std::string &condition);
        [[nodiscard]] uint64_t expression(const std::string &e);
        void mark_as_exited();
        [[nodiscard]] bool has_conditional_breakpoint(uint64_t address) const;
        void set_fpu_register(const std::string &text, double value);
        [[nodiscard]] double get_fpu_register(const std::string &text);
        [[nodiscard]] std::string print_fpu_registers();
        [[nodiscard]] std::string hex_dump(uint64_t address, uint64_t size);
        void set_pc(uint64_t address);

      private:
        explicit Process(pid_t pid) : m_pid(pid), current_thread_id(pid) {}
        pid_t m_pid, current_thread_id;
        bool is_single_stepping = false;
        std::map<uint64_t, uint8_t> breakpoints;
        std::vector<uint64_t> breakpoint_index;
        std::map<uint64_t, ConditionalBreakpoint> conditional_breakpoints;
        void handle_breakpoint_step(uint64_t address);
        void handle_conditional_breakpoint_continue(uint64_t address, bool continue_after_step = true);

        void set_fpu_registers(const user_fpregs_struct &fpregs);
        [[nodiscard]] user_fpregs_struct get_fpu_registers() const;

        size_t index_{};
        std::vector<Watchpoint> watchpoints_;
        bool exited_ = false;
        mutable uint64_t skip_next_breakpoint = 0;
        std::optional<std::pair<uint64_t, uint8_t>> stepped_breakpoint_;
        StopInfo last_stop_;
        std::uint64_t stop_sequence_ = 0;
    };

} // namespace mx

#endif
