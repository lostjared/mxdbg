.section .data
    hello_msg: .asciz "Hello World"
    print_digits: .asciz "The number: %d %d\n"

.section .text
    .global main

main:
    push %rbp           
    mov %rsp, %rbp      

    sub $16, %rsp
    movq $0, -0x8(%rbp)

    lea hello_msg(%rip), %rdi    # First argument for puts
    call puts          

    lea print_digits(%rip), %rdi 
    mov $255, %rsi               
    call printf


    mov -0x8(%rbp), %rbx
loop_1:
    lea print_digits(%rip), %rdi
    mov %rbx, %rsi
    mov $1024, %rdx
    call print_number
    inc %rbx
    cmp $10, %rbx
    jle loop_1
    mov %rbx, %rax
    mov %rbp, %rsp       
    pop %rbp            
    ret                 

print_number:
    push %rbp
    mov %rsp, %rbp
    call printf
    mov $0, %eax
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

