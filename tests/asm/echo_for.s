.section .data
ch_dat: .asciz "%c : %c\n"
.section .bss
.section .text
.global echo_for

echo_for:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %r12
    mov %rdi, %r15
    call strlen
    mov %rax, %r13
    add %rax, %r12
    dec %r12
    dec %r13
print_loop:
    movb (%r12), %al
    movzx %al, %esi
    lea ch_dat(%rip), %rdi
    movb (%r15), %al
    movzx %al, %edx
    call printf
    dec %r12
    dec %r13
    inc %r15
    cmp $-1, %r13
    jg print_loop
done:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits 


