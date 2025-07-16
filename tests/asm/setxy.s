.section .data
ctrl_code:.asciz  "\033[%d;%dH"
.section .text
.global setxy

setxy:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %r12
    mov %rsi, %r13
    mov %rdx, %r14
    lea ctrl_code(%rip), %rdi
    mov %r12, %rsi
    mov %r14, %rdx
    xor %rax, %rax
    call printf
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits



