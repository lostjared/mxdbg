.section .data



.section .rodata
    num1: .double 5.5
    num2: .double 6.1
    num3: .double 2.6
    output_str: .asciz "(%.02f + %.02f) * %.02f = %.02f\n"
.section .bss

.section .text

    .global main
    .extern printf

main:
    push %rbp
    mov %rsp, %rbp
    movsd num1(%rip), %xmm0
    movsd num2(%rip), %xmm1
    movsd num3(%rip), %xmm2

    addsd %xmm1, %xmm0
    mulsd %xmm0, %xmm2
    movsd %xmm2, %xmm3

    movsd num1(%rip), %xmm0
    movsd num2(%rip), %xmm1
    movsd num3(%rip), %xmm2

    mov $4, %rax
    lea output_str(%rip), %rdi
    call printf    

    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

