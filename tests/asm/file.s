.section .data
file_string: .asciz "Hello, World\n"
file_name: .asciz "hello.txt"
file_mode: .asciz "w"

.section  .bss
.section .text
.global main

main:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    lea file_name(%rip), %rdi
    lea file_mode(%rip), %rsi
    call fopen
    cmp $0, %rax
    je exit_fail
    mov %rax, -0x8(%rbp)
    mov %rax, %rdi
    lea file_string(%rip),%rsi
    xor %rax, %rax
    call fprintf
cleanup:
    mov -0x8(%rbp), %rdi
    call fclose
exit_main:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret
exit_fail:
    mov $1, %rax  
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits

