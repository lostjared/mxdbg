.section .data
    num1: .quad 8
    num2: .quad 4
    result: .quad 0
    newline: .byte 10

.section .bss
    buffer: .space 32

.section .text
.global _start

_start:
    # Load numbers and multiply
    movq num1(%rip), %rax
    movq num2(%rip), %rbx
    mulq %rbx
    movq %rax, result(%rip)

    # Convert result to string
    movq result(%rip), %rax
    leaq buffer(%rip), %rsi
    addq $31, %rsi          # Point to end of buffer
    movb $0, (%rsi)         # Null terminate
    decq %rsi

    movq $10, %rcx          # Base 10
convert_loop:
    xorq %rdx, %rdx
    divq %rcx
    addq $'0', %rdx         # Convert remainder to ASCII
    movb %dl, (%rsi)
    decq %rsi
    testq %rax, %rax
    jnz convert_loop

    incq %rsi               # Point to first digit

    # Calculate string length
    leaq buffer+31(%rip), %rdi
    subq %rsi, %rdi         # Length in %rdi
    movq %rdi, %rdx         # Save length before overwriting %rdi

    # Write result using sys_write
    movq $1, %rax           # sys_write
    movq $1, %rdi           # stdout
    syscall

    # Write newline
    movq $1, %rax           # sys_write
    movq $1, %rdi           # stdout
    leaq newline(%rip), %rsi
    movq $1, %rdx           # Length
    syscall

    # Exit
    movq $60, %rax          # sys_exit
    movq $0, %rdi           # Exit status
    syscall

.section .note.GNU-stack,"",@progbits

