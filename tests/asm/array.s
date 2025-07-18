.section .data
    prompt_msg: .asciz "Enter 5 numbers:\n"
    input_prompt: .asciz "Enter number %d: "
    output_format: .asciz "Number %d: %d\n"
    scan_format: .asciz "%d"
    .align 4
    array: .space 20     # Reserve space for 5 integers (4 bytes each)

.section .text
.globl main

main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp           # Reserve 16 bytes for local variables (loop counter at -4(%rbp))

    # Display initial prompt
    lea prompt_msg(%rip), %rdi
    xor %eax, %eax
    call printf

    movl $0, -4(%rbp)       # i = 0
input_loop:
    movl -4(%rbp), %eax
    cmpl $5, %eax
    jge print_array

    lea input_prompt(%rip), %rdi
    movl -4(%rbp), %esi
    addl $1, %esi
    xor %eax, %eax
    call printf

    lea scan_format(%rip), %rdi
    movl -4(%rbp), %eax
    leaq array(,%rax,4), %rsi
    xor %eax, %eax
    call scanf

    movl -4(%rbp), %eax
    incl %eax
    movl %eax, -4(%rbp)
    jmp input_loop

print_array:
    lea prompt_msg(%rip), %rdi
    xor %eax, %eax
    call printf

    movl $0, -8(%rbp)       # j = 0
output_loop:
    movl -8(%rbp), %eax
    cmpl $5, %eax
    jge exit

    lea output_format(%rip), %rdi
    movl -8(%rbp), %esi
    addl $1, %esi
    movl -8(%rbp), %eax
    movl array(,%rax,4), %edx
    xor %eax, %eax
    call printf

    movl -8(%rbp), %eax
    incl %eax
    movl %eax, -8(%rbp)
    jmp output_loop

exit:
    mov $0, %eax
    leave
    ret

.section .note.GNU-stack,"",@progbits




