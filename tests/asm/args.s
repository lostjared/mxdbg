.section .data
.section .rodata
    output_text: .asciz "argv[%ld]: -> %s\n"
    output_args: .asciz "Argument count: %ld\n"
.section .bss

.section .text
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    push %rdi
    push %rsi
    mov %rdi, %rsi
    lea output_args(%rip), %rdi
    call printf
    pop %rsi
    pop %rdi
    mov %rdi, %rcx
    mov %rdi, %r8
    mov %rsi, %rax
    mov $0, %rcx
.loop:
    cmp %r8, %rcx # rcx >= r8
    jge .over
    push %rax
    movl %ecx, %esi
    lea output_text(%rip), %rdi
    mov (%rax, %rcx, 8), %rdx
    push %rcx
    push %r8
    xor %rax, %rax
    call printf
    pop %r8
    pop %rcx
    inc %rcx
    pop %rax
    jmp .loop
.over:
    mov %rsp, %rbp
    pop %rbp

    ret

    .section .note.GNU-stack, "",@progbits
