.section .data
print_char: .asciz "%c"

.section .bss
.section .text
.global trim

trim:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    mov %rdi, -0x8(%rbp)
    mov %rsi, %r12
loop_chars:
    movb (%r12), %al
    inc %r12
    mov -0x8(%rbp), %rdi
    lea print_char(%rip), %rsi
    movzx %al, %edx
    cmpb $0x23, %al
    je skip_chars
    test %al, %al
    jz end_loop
    xor %rax, %rax
    call fprintf
    jmp loop_chars    
skip_chars:
    movb (%r12), %al
    cmpb $0xa, %al
    inc %r12
    je loop_chars
    test %al, %al
    jz end_loop
    jmp skip_chars
end_loop:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits



    
    
