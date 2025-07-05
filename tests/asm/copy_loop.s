.section .data
    variable1: .asciz "Value is One \n"
    variable2: .asciz "Value is Two \n"
.section .bss 
    copy_string: .space 256
.section .text
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    lea variable1(%rip), %rdi
    call strlen
    mov %rax, %r8
    lea copy_string(%rip), %rsi
    mov %rsi, %rcx
    add %r8, %rcx
    movb $0, (%rcx)
copy_loop:
    movb (%rdi), %al
    movb %al, (%rsi)
    test %al, %al
    jz copy_done
    inc %rsi
    inc %rdi
    jmp copy_loop
copy_done:
    lea copy_string(%rip), %rdi
    call strlen
    lea copy_string(%rip), %rcx
    add %rax, %rcx
    lea variable2(%rip), %rsi   
copy_loop2:
    movb (%rsi), %al
    movb %al, (%rcx)
    test %al, %al
    jz copy_done2
    inc %rcx
    inc %rsi
    jmp copy_loop2
copy_done2:
    lea copy_string(%rip), %rdi
    call printf
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret

.section .note.GNU-stack,"",@progbits


