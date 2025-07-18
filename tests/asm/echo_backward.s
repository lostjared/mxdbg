.section .data
ch_dat: .asciz "%c\n"
.section .bss
.section .text
.global echo_backwards

echo_backwards:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    sub $16, %rsp
    mov %rdi, %r12
    call strlen 
    mov %rax, %r13
    add %rax, %r12
    dec %r12
    dec %r13
print_loop:
    movb (%r12), %al
    movzx %al, %esi
    lea ch_dat(%rip), %rdi
    xor %rax, %rax
    call printf
    dec %r12
    dec %r13
    cmp $-1,%r13
    jg print_loop
done:
    pop %r13
    pop %r12
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits

