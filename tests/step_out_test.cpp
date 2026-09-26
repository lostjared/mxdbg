#include "mxdbg/debugger.hpp"

#include <cassert>
#include <filesystem>
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
