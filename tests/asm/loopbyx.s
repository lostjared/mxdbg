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
    sub $32, %rsp
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
    movq $0,-0x8(%rbp)
    lea -0x8(%rbp), %rsi
    call scanf
    mov -0x8(%rbp), %r12
    mov $0, %rbx
loopbyx:
    cmp %rbx, %r12
    je exit_main
    lea input_buffer(%rip), %rdi
    xor %rax, %rax
    call printf
    inc %rbx
    jmp loopbyx
exit_main:
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret
exit_error:
    mov %rbp, %rsp
    pop %rbp
    mov $-1, %rax
    ret

.section .note.GNU-stack,"",@progbits

