.section .data
    hello_msg: .asciz "Hello World"
    print_digits: .asciz "The number: %d %d\n"

.section .text
    .global main

main:
    push %rbp
    mov %rsp, %rbp
    lea hello_msg(%rip), %rdi    # First argument for puts
    call puts
    lea print_digits(%rip), %rdi
    mov $255, %rsi
    mov $0, %rdx
    xor %rax, %rax
    call printf
    mov $0, %rbx
loop_1:
    lea print_digits(%rip), %rdi
    mov %rbx, %rsi
    mov $1024, %rdx
    call print_number
    inc %rbx
    cmp $10, %rbx
    jle loop_1
    mov %rbp, %rsp
    pop %rbp
    mov $0, %eax
    ret

print_number:
    push %rbp
    mov %rsp, %rbp
    xor %rax, %rax
    call printf
    mov $0, %eax
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

