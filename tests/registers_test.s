# Test program for register manipulation in GNU AT&T syntax
# This program sets specific values in registers and provides breakpoint locations
# for testing the debugger's register read/write functionality

.section .text
.global _start

_start:
    # Initialize registers with known test values
    movq $0x1122334455667788, %rax    # Test RAX with 64-bit value
    movq $0x8877665544332211, %rbx    # Test RBX with 64-bit value
    movq $0xabcdef0123456789, %rcx    # Test RCX with 64-bit value
    movq $0x9876543210fedcba, %rdx    # Test RDX with 64-bit value
    
    # Breakpoint location 1 - test 64-bit registers
    nop                               # <-- Set breakpoint here to test 64-bit reads
    
    # Test 32-bit register operations
    movl $0x12345678, %eax            # Test EAX (clears upper 32 bits of RAX)
    movl $0x87654321, %ebx            # Test EBX (clears upper 32 bits of RBX)
    movl $0xabcdef12, %ecx            # Test ECX (clears upper 32 bits of RCX)
    movl $0x12fedcba, %edx            # Test EDX (clears upper 32 bits of RDX)
    
    # Breakpoint location 2 - test 32-bit registers
    nop                               # <-- Set breakpoint here to test 32-bit reads
    
    # Test 16-bit register operations
    movw $0x1234, %ax                 # Test AX
    movw $0x5678, %bx                 # Test BX
    movw $0xabcd, %cx                 # Test CX
    movw $0xef12, %dx                 # Test DX
    
    # Breakpoint location 3 - test 16-bit registers
    nop                               # <-- Set breakpoint here to test 16-bit reads
    
    # Test 8-bit register operations
    movb $0x12, %al                   # Test AL
    movb $0x34, %ah                   # Test AH
    movb $0x56, %bl                   # Test BL
    movb $0x78, %bh                   # Test BH
    movb $0xab, %cl                   # Test CL
    movb $0xcd, %ch                   # Test CH
    movb $0xef, %dl                   # Test DL
    movb $0x12, %dh                   # Test DH
    
    # Breakpoint location 4 - test 8-bit registers
    nop                               # <-- Set breakpoint here to test 8-bit reads
    
    # Test additional registers (RSI, RDI, RSP, RBP)
    movq $0x1234567890abcdef, %rsi    # Test RSI
    movq $0xfedcba0987654321, %rdi    # Test RDI
    
    # Save current stack pointer and set test value
    pushq %rbp                        # Save original RBP
    movq %rsp, %rbp                   # Set up stack frame
    movq $0x1111222233334444, %r8     # Test R8
    movq $0x5555666677778888, %r9     # Test R9
    
    # Breakpoint location 5 - test extended registers
    nop                               # <-- Set breakpoint here to test RSI, RDI, R8, R9
    
    # Test all R8-R15 registers
    movq $0xaaaa000000000001, %r8
    movq $0xbbbb000000000002, %r9
    movq $0xcccc000000000003, %r10
    movq $0xdddd000000000004, %r11
    movq $0xeeee000000000005, %r12
    movq $0xffff000000000006, %r13
    movq $0x1111000000000007, %r14
    movq $0x2222000000000008, %r15
    
    # Breakpoint location 6 - test R8-R15 registers
    nop                               # <-- Set breakpoint here to test R8-R15
    
    # Test register arithmetic to verify values are correct
    addq %rax, %rbx                   # RBX = RAX + RBX
    subq %rcx, %rdx                   # RDX = RDX - RCX
    
    # Breakpoint location 7 - test after arithmetic
    nop                               # <-- Set breakpoint here to test modified values
    
    # Test flag manipulation
    cmpq %rax, %rbx                   # Compare RAX and RBX (sets flags)
    
    # Breakpoint location 8 - test flags
    nop                               # <-- Set breakpoint here to test EFLAGS
    
    # Restore and exit
    popq %rbp                         # Restore original RBP
    
    # Exit system call
    movq $60, %rax                    # sys_exit
    movq $0, %rdi                     # exit status
    syscall

# Data section for reference values
.section .data
test_values:
    .quad 0x1122334455667788          # Expected RAX value
    .quad 0x8877665544332211          # Expected RBX value
    .quad 0xabcdef0123456789          # Expected RCX value
    .quad 0x9876543210fedcba          # Expected RDX value
    
test_32bit:
    .long 0x12345678                  # Expected EAX value
    .long 0x87654321                  # Expected EBX value
    .long 0xabcdef12                  # Expected ECX value
    .long 0x12fedcba                  # Expected EDX value

test_16bit:
    .word 0x1234                      # Expected AX value
    .word 0x5678                      # Expected BX value
    .word 0xabcd                      # Expected CX value
    .word 0xef12                      # Expected DX value

test_8bit:
    .byte 0x12, 0x34                  # Expected AL, AH values
    .byte 0x56, 0x78                  # Expected BL, BH values
    .byte 0xab, 0xcd                  # Expected CL, CH values
    .byte 0xef, 0x12                  # Expected DL, DH values

    