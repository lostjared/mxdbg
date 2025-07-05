.section .data

.section .bss

.section .text
.global string_cat

string_cat:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %r12
    mov %rsi, %r13
    call strlen
    mov %rax, %r8
    mov %rax, %r14
    mov %r13, %rdi
    call strlen
    add %rax, %r8
    add %r12, %r8
    movb $0, (%r8)
    add %r14, %r12
copy_loop:
    movb (%r13), %al
    movb %al, (%r12)
    test %al, %al
    jz copy_done
    inc %r13
    inc %r12
    jmp copy_loop
copy_done:
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret

.section .note.GNU-stack,"",@progbits

