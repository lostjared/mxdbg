#include "mxdbg/debug_context.hpp"

#include <algorithm>
#include <format>
#include <sstream>
#include <string_view>

namespace mx {
    DebugContext::DebugContext(std::size_t limit)
        : limit_(std::max<std::size_t>(limit, 1024)) {}

    void DebugContext::clear() {
        events_.clear();
        instructions_.clear();
        insights_.clear();
        crash_report_.clear();
        omitted_instructions_ = 0;
    }

    void DebugContext::add_event(const std::string& event) {
        events_.push_back(event);
        trim();
    }

    void DebugContext::add_instruction(std::uint64_t address, const std::string& instruction) {
        instructions_.push_back(std::format("0x{:016X}: {}", address, instruction));
        trim();
    }

    void DebugContext::add_insight(const std::string& title, const std::string& insight) {
        insights_.push_back(title + "\n" + insight);
        trim();
    }

    void DebugContext::add_crash(const std::string& crash_report) {
        crash_report_ = crash_report;
        if (crash_report_.size() > limit_) {
            static constexpr std::string_view marker = "\n[crash report truncated to context limit]\n";
            crash_report_.resize(limit_ - marker.size());
            crash_report_ += marker;
        }
        trim();
    }

    void DebugContext::clear_crash() {
        crash_report_.clear();
    }

    std::size_t DebugContext::size() const {
        std::size_t total = crash_report_.size();
        for (const auto& value : events_) total += value.size();
        for (const auto& value : instructions_) total += value.size();
        for (const auto& value : insights_) total += value.size();
        return total;
    }

    void DebugContext::trim() {
        while (size() > limit_ && !instructions_.empty()) {
            instructions_.pop_front();
            ++omitted_instructions_;
        }
        while (size() > limit_ && !events_.empty()) {
            events_.pop_front();
        }
        while (size() > limit_ && !insights_.empty()) {
            insights_.pop_front();
        }
    }

    std::string DebugContext::render_evidence() const {
        std::ostringstream output;
        output << "DEBUG SESSION EVIDENCE\n";
        if (!crash_report_.empty()) {
            output << "\nCRASH SNAPSHOT (highest priority)\n"
                   << crash_report_ << "\n";
        }
        if (!events_.empty()) {
            output << "\nRECENT EVENTS\n";
            for (const auto& event : events_) output << "- " << event << "\n";
        }
        if (!instructions_.empty()) {
            output << "\nRECENT INSTRUCTIONS\n";
            for (const auto& instruction : instructions_) output << instruction << "\n";
        }
        if (omitted_instructions_ != 0) {
            output << "[" << omitted_instructions_ << " older instruction(s) omitted]\n";
        }
        std::string rendered = output.str();
        if (rendered.size() > limit_) {
            rendered.resize(limit_);
        }
        return rendered;
    }

    std::string DebugContext::render() const {
        std::string evidence = render_evidence();
        static constexpr std::string_view evidence_header = "DEBUG SESSION EVIDENCE";
        static constexpr std::string_view context_header = "DEBUG SESSION CONTEXT";
        if (evidence.starts_with(evidence_header)) {
            evidence.replace(0, evidence_header.size(), context_header);
        }
        std::ostringstream output;
        output << evidence;
        if (!insights_.empty()) {
            output << "\nPRIOR AI INSIGHTS\n";
            for (const auto& insight : insights_) output << insight << "\n";
        }
        std::string rendered = output.str();
        if (rendered.size() > limit_) {
            rendered.resize(limit_);
        }
        return rendered;
    }
}
