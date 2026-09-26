#include "mxdbg/debug_context.hpp"

#include <cassert>
#include <string>

int main() {
    mx::DebugContext context(1024);
    context.add_event("program launched");
    for (std::uint64_t address = 0; address < 100; ++address) {
        context.add_instruction(address, std::string(40, 'x'));
    }
    context.add_crash("SIGSEGV at 0xDEADBEEF\nRIP=0x1234\nstack frame: main");

    const std::string rendered = context.render();
    assert(rendered.find("CRASH SNAPSHOT") != std::string::npos);
    assert(rendered.find("SIGSEGV at 0xDEADBEEF") != std::string::npos);
    assert(context.omitted_instructions() > 0);
    assert(context.size() <= context.limit());
    assert(rendered.size() <= context.limit());

    context.add_insight("analysis", std::string(2000, 'y'));
    assert(context.render().find("SIGSEGV at 0xDEADBEEF") != std::string::npos);

    context.clear();
    assert(context.size() == 0);
    assert(context.render().find("SIGSEGV") == std::string::npos);

    context.add_crash("SIGSEGV at 0x0");
    assert(context.has_active_crash());
    context.clear_crash();
    assert(!context.has_active_crash());
    assert(context.render().find("SIGSEGV") == std::string::npos);
}
