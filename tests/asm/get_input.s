.section .data
    input_message: .asciz "Enter a number: "
    input_string: .asciz "%d"
    output_message: .asciz "The number is: %d\n"
    is_value_message: .asciz "value is 10\n"
.section .text
    .global main

main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    movq $0, -0x8(%rbp)
    lea input_message(%rip), %rdi
    xor %rax, %rax
    call printf
    lea input_string(%rip), %rdi
    lea -0x8(%rbp), %rsi
    call scanf
    lea output_message(%rip), %rdi
    mov -0x8(%rbp), %rsi
    xor %rax, %rax
    call printf
    mov -0x8(%rbp), %rax
    cmp $10, %rax
    jne not_value
    call  is_value
not_value:
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret 
is_value:
    push %rbp
    mov %rsp, %rbp
    lea is_value_message(%rip), %rdi
    call printf
    mov %rbp, %rsp
    pop %rbp
    ret
.section .note.GNU-stack,"",@progbits

