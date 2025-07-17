.section .data
    .extern stdin
    num1: .double 0
    num2: .double 0
.section .rodata
    input_string: .asciz "Input a  Number: "
    output_string: .asciz "%0.2f + %0.2f = %0.2f\n"
    max_size: .long 255
.section .bss
    .lcomm input_buffer, 256
.section .text
    .global main
    .extern printf, fgets, atof
main:
    push %rbp
    mov %rsp,  %rbp
    call input_number
    movsd %xmm0, num1(%rip)
    call input_number
    movsd %xmm0, num2(%rip)
    call add_two
    movsd %xmm0, %xmm2
    movsd num1(%rip), %xmm0
    movsd num2(%rip), %xmm1
    lea output_string(%rip), %rdi
    mov $3, %rax
    call printf
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret
input_number:
    push %rbp
    mov %rsp, %rbp
    lea input_string(%rip), %rdi
    call printf
    lea input_buffer(%rip), %rdi
    movl max_size(%rip), %esi
    movq stdin(%rip), %rdx
    call fgets
    lea input_buffer(%rip), %rdi
    call atof
    mov %rbp, %rsp
    pop %rbp
    ret
add_two:
    push %rbp
    mov %rsp, %rbp
    movsd num1(%rip), %xmm0
    movsd num2(%rip), %xmm1
    addsd %xmm1, %xmm0
    mov %rbp, %rsp
    pop %rbp
    ret


.section .note.GNU-stack,"",@progbits


