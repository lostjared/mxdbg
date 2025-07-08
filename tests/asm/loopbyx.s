.section .data
    input_message: .asciz "Enter number: "
    input_scan: .asciz "%d"
    message: .asciz "Hello, World\n"

.section .text
    .global main

main:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    lea input_message(%rip), %rdi
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
    lea message(%rip), %rdi
    call printf
    inc %rbx
    jmp loopbyx
exit_main:
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret

.section .note.GNU-stack,"",@progbits

