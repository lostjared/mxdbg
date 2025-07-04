.section .data
    input_message: .asciz "Enter a string: \n"
    output_message: .asciz "The string is: %s\n"

.section .text
    .global main

main:
    push %rbp
    mov %rsp, %rbp
    sub $256, %rsp
    lea input_message(%rip), %rdi
    call printf
    mov $0, %rax
    lea input_buffer(%rip), %rsi
    mov $256, %rdx
    mov $0, %rdi
    call read
    lea output_message(%rip), %rdi
    lea input_buffer(%rip), %rsi
    call printf
    mov %rbp, %rsp
    pop %rbp
    movl $0, %eax
    ret

.section .note.GNU-stack,"",@progbits

.section .bss
    .lcomm input_buffer, 256  

