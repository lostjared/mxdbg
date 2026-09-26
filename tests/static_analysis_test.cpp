#include "mxdbg/static_analysis.hpp"

#include <cassert>
#include <string>

int main() {
    const auto& signatures = mx::known_function_signatures();
    for (const char* name : {"printf", "fprintf", "fopen", "fread", "fwrite", "atoi",
                             "putchar", "fflush", "usleep", "fclose", "exit"})
        assert(signatures.contains(name));

    const std::string disassembly = R"(0000000000401186 <main>:
  401186:	55                   	push   %rbp
  401187:	48 89 e5             	mov    %rsp,%rbp
  40118a:	89 7d dc             	mov    %edi,-0x24(%rbp)
  40118d:	48 89 75 d0          	mov    %rsi,-0x30(%rbp)
  401191:	83 7d dc 03          	cmpl   $0x3,-0x24(%rbp)
  401195:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  401199:	48 83 c0 08          	add    $0x8,%rax
  40119d:	48 8b 00             	mov    (%rax),%rax
  4011a0:	48 8d 15 5d 0e 00 00 	lea    0xe5d(%rip),%rdx # 402004 <_IO_stdin_used+0x4>
  4011a7:	48 89 d6             	mov    %rdx,%rsi
  4011aa:	48 89 c7             	mov    %rax,%rdi
  4011ad:	e8 ae fe ff ff       	call   401060 <fopen@plt>
  4011b2:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  4011b6:	48 8d 05 83 2e 00 00 	lea    0x2e83(%rip),%rax # 404040 <stderr@GLIBC_2.2.5>
  4011bd:	48 89 c1             	mov    %rax,%rcx
  4011c0:	ba 19 00 00 00       	mov    $0x19,%edx
  4011c5:	be 01 00 00 00       	mov    $0x1,%esi
  4011ca:	48 8d 05 45 0e 00 00 	lea    0xe45(%rip),%rax # 402016 <message>
  4011d1:	48 89 c7             	mov    %rax,%rdi
  4011d4:	e8 77 fe ff ff       	call   401050 <fwrite@plt>
  4011d9:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  4011dd:	48 83 c0 10          	add    $0x10,%rax
  4011e1:	48 8b 00             	mov    (%rax),%rax
  4011e4:	48 89 c7             	mov    %rax,%rdi
  4011e7:	e8 44 fe ff ff       	call   401030 <atoi@plt>
  4011ec:	89 45 e4             	mov    %eax,-0x1c(%rbp)
  4011ef:	8b 45 e4             	mov    -0x1c(%rbp),%eax
  4011f2:	69 c0 e8 03 00 00    	imul   $0x3e8,%eax,%eax
  4011f8:	89 c7                	mov    %eax,%edi
  4011fa:	e8 31 fe ff ff       	call   401030 <usleep@plt>
  4011ff:	c6 45 e3 00          	movb   $0x0,-0x1d(%rbp)
  401203:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  401207:	ba 01 00 00 00       	mov    $0x1,%edx
  40120c:	be 01 00 00 00       	mov    $0x1,%esi
  401211:	48 8d 7d e3          	lea    -0x1d(%rbp),%rdi
  401215:	e8 46 fe ff ff       	call   401060 <fread@plt>
  40121a:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
)";

    const std::string facts = mx::derive_static_facts("main", disassembly);
    assert(facts.find("compared against 3") != std::string::npos);
    assert(facts.find("[rbp-0x18] = return value of fopen") != std::string::npos);
    assert(facts.find("[rbp-0x1c] = return value of atoi") != std::string::npos);
    assert(facts.find("filename = argv[1]") != std::string::npos);
    assert(facts.find("ptr = constant pointer message") != std::string::npos);
    assert(facts.find("size = 1") != std::string::npos);
    assert(facts.find("nmemb = 25") != std::string::npos);
    assert(facts.find("stream = stderr") != std::string::npos);
    assert(facts.find("interpreted call: fwrite(constant pointer message, 1, 25, stderr)") != std::string::npos);
    assert(facts.find("string = argv[2]") != std::string::npos);
    assert(facts.find("microseconds = return value of atoi * 1000") != std::string::npos);
    assert(facts.find("ptr = [rbp-0x1d]") != std::string::npos);
    assert(facts.find("[rbp-0x1d] = one-byte buffer") != std::string::npos);
    assert(facts.find("[rbp-0x10] = return value of fread") != std::string::npos);
    assert(facts.find("[rbp-0x1d] = 0") == std::string::npos);
}
