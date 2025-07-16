.section .data

.section .rodata
    output_str: .asciz "Value is: %f\n"
    num1: .float 1.5
    num2: .float 2.5
.section .bss
    .lcomm value, 4

.section .text
    .global main
    .extern printf

main:
    push %rbp   
    mov %rsp, %rbp
    movss   num1(%rip), %xmm0 # load float value into register
    movss   num2(%rip), %xmm1 # load float value into register
    addss   %xmm1, %xmm0 # add two float values
    movss %xmm0, value(%rip) # save to value
    cvtss2sd %xmm0, %xmm0 # convert to double
    lea output_str(%rip), %rdi 
    mov $1, %rax # tell printf we want to print a float
    call printf # call printf
    movss value(%rip), %xmm1 # load from value
    movss num2(%rip), %xmm0  # load form num2
    addss %xmm1, %xmm0 # add frp,
    mulss %xmm1, %xmm0 # mul
    mov $1, %rax
    lea output_str(%rip), %rdi
    cvtss2sd %xmm0, %xmm0
    call printf
    mov $0, %rax
    mov %rbp,%rsp # clean up stack
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

