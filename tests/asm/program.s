.section .data
    hello_msg: .asciz "Hello World"
    print_digits: .asciz "The number: %d\n"

.section .text
    .global main

main:
    push %rbp           
    mov %rsp, %rbp      
    
    lea hello_msg(%rip), %rdi    # First argument for puts
    call puts

    lea print_digits(%rip), %rdi 
    mov $255, %rsi               
    call printf

    lea print_digits(%rip), %rdi
    mov $1024, %rsi
    call print_number

    mov $0, %eax        
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

