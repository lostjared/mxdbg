.section .data
    buffer_message: .asciz "Enter string: \n"
    input_message: .asciz "Enter number: "
    input_scan: .asciz "%d"
    message: .asciz "Hello, World\n"
.section .bss
    input_buffer: .space 256
.section .text
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp

    lea buffer_message(%rip), %rdi
    xor %rax, %rax
    call printf

    mov $0, %rax
    mov $0, %rdi
    lea input_buffer(%rip), %rsi
    mov $255, %rdx
    syscall
    test %rax, %rax
    jle exit_error

    lea input_message(%rip), %rdi
    xor %rax, %rax
    call printf

    lea input_scan(%rip), %rdi
    movq $0, -8(%rbp)
    lea -8(%rbp), %rsi
    call scanf

    movq $0, -16(%rbp)

loopbyx:
    movq -16(%rbp), %rax
    movq -8(%rbp), %rdx
    cmp %rax, %rdx
    je exit_main

    lea input_buffer(%rip), %rdi
    xor %rax, %rax
    call printf

    movq -16(%rbp), %rax
    inc %rax
    movq %rax, -16(%rbp)
    jmp loopbyx

exit_main:
    movl $0, %eax
    leave
    ret

exit_error:
    mov $-1, %rax
    leave
    ret

.section .note.GNU-stack,"",@progbits

