#include "mxdbg/debugger.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {
    mx::StepOutResult run_step_out(const std::filesystem::path& fixture,
                                   std::string_view arguments) {
        mx::Debugger debugger(false);
        assert(debugger.launch(fixture, arguments));
        assert(debugger.command("run"));
        assert(debugger.command("function step_out_target"));
        assert(debugger.command("continue"));
        const mx::StepOutResult result = debugger.step_out();
        if (result == mx::StepOutResult::Signal) {
            std::ostringstream captured;
            std::streambuf *original = std::cout.rdbuf(captured.rdbuf());
            assert(debugger.command("backtrace"));
            std::cout.rdbuf(original);
            const std::string backtrace = captured.str();
            assert(backtrace.find("_fini+") == std::string::npos);
            assert(backtrace.find("libc.so") != std::string::npos ||
                   backtrace.find("__libc") != std::string::npos);
        }
        if (debugger.is_running()) debugger.detach();
        return result;
    }
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::filesystem::path fixture(argv[1]);
    assert(run_step_out(fixture, "") == mx::StepOutResult::Completed);
    assert(run_step_out(fixture, "crash") == mx::StepOutResult::Signal);
}
