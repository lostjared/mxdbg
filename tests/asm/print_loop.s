.section .data
ch_dat: .asciz "%c\n"
.section .bss
.section .text
.global echo_text

echo_text:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %r12
loop_test:
    lea ch_dat(%rip), %rdi
    movb (%r12), %al
    movl %eax, %esi
    test %al, %al
    jz done
    xor %rax, %rax
    call printf
    inc %r12
    jmp loop_test
done:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits


