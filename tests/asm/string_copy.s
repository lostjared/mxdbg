# string_copy 

# rdi - dest
# rsi - source
# rdx - max buffer size

.section .data
.section .bss
.section .text
.global string_copy

string_copy:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    push %r15
    sub $8, %rsp
    
    mov %rdi, %r12
    mov %rsi, %r13
    mov %rsi, %rdi
    mov %rdx, %r15
    call strlen
    mov %rax, %r14
    inc %rax
    cmp %r15, %rax
    jge skip_copy
    add %r12, %r14
    movb $0, (%r14)
copy_loop:
    movb (%r13), %al
    movb %al, (%r12)
    test %al, %al
    jz done
    inc %r12
    inc %r13
    jmp copy_loop
skip_copy:
    mov $-1, %rax
    jmp cleanup
done:
    movl $0, %eax
cleanup:
    add $8, %rsp
    pop %r15
    pop %r13
    pop %r12
    leave
    ret

.section .note.GNU-stack, "",@progbits
