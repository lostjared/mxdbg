#ifndef MXDBG_DEBUG_CONTEXT_HPP
#define MXDBG_DEBUG_CONTEXT_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace mx {
    class DebugContext {
      public:
        explicit DebugContext(std::size_t limit = 32768);

        void clear();
        void add_event(const std::string& event);
        void add_instruction(std::uint64_t address, const std::string& instruction);
        void add_insight(const std::string& title, const std::string& insight);
        void add_crash(const std::string& crash_report);
        void clear_crash();

        [[nodiscard]] std::string render() const;
        [[nodiscard]] bool has_active_crash() const { return !crash_report_.empty(); }
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::size_t limit() const { return limit_; }
        [[nodiscard]] std::size_t omitted_instructions() const { return omitted_instructions_; }

      private:
        void trim();

        std::size_t limit_;
        std::size_t omitted_instructions_ = 0;
        std::deque<std::string> events_;
        std::deque<std::string> instructions_;
        std::deque<std::string> insights_;
        std::string crash_report_;
    };
}

#endif
