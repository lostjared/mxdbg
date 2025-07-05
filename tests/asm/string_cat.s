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
    mov %rdx, %r15

    call strlen
    mov %rax, %r14

    mov %r13, %rdi
    call strlen

    add %r14, %rax
    inc %rax
    cmp %r15, %rax
    jge skip_copy
    add %r14, %r12

copy_loop:
     movb (%r13), %al
     movb %al, (%r12)
     test %al, %al
     jz done
     inc %r13
     inc %r12
     jmp copy_loop
skip_copy:
    mov $-1, %rax
    jmp cleanup
done:
    mov $0, %rax
cleanup:
    mov %rbp, %rsp 
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits




