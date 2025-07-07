.section .data
print_char: .asciz "%c"

.section .bss
.section .text
.global trim

trim:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %r12
loop_chars:
    movb (%r12), %al
    inc %r12
    lea print_char(%rip), %rdi
    movzx %al, %esi
    cmpb $0x23, %al
    je skip_chars
    test %al, %al
    jz end_loop
    call printf
    jmp loop_chars    
skip_chars:
    movb (%r12), %al
    cmpb $0xa, %al
    je loop_chars
    inc %r12
    test %al, %al
    jz end_loop
    jmp skip_chars
end_loop:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits



    
    
