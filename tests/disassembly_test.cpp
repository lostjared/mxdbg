#include "mxdbg/disassembly.hpp"

#include <cassert>
#include <string>

int main() {
    const std::string disassembly = R"(
Disassembly of section .text:

0000000000401000 <caller>:
  401000:	e8 0b 00 00 00       	call   401010 <print_number>
  401005:	c3                   	ret

0000000000401010 <print_number>:
  401010:	48 83 f8 00          	cmp    $0x0,%rax
  401014:	75 04                	jne    40101a <convert_loop>
  401016:	eb 0a                	jmp    401022 <print_string>

000000000040101a <convert_loop>:
  40101a:	48 ff c8             	dec    %rax
  40101d:	75 fb                	jne    40101a <convert_loop>

0000000000401022 <print_string>:
  401022:	c3                   	ret
  401023:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)

0000000000401030 <next_function>:
  401030:	c3                   	ret
)";

    const auto result = mx::extract_function_disassembly(disassembly, "print_number");
    assert(result.find("<print_number>:") != std::string::npos);
    assert(result.find("<convert_loop>:") != std::string::npos);
    assert(result.find("<print_string>:") != std::string::npos);
    assert(result.find("<next_function>:") == std::string::npos);

    assert(mx::extract_function_disassembly(disassembly, "missing").empty());
}
