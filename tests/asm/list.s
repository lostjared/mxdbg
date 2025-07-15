.section .data

.align 8
list_node:
.quad 0          
.quad 0          

.section .bss
    .comm list_node_ptr, 8, 8

.section .rodata
fmt:
    .asciz "%ld\n"

.section .text
    .global main
    .extern printf, malloc    
main:
    push %rbp
    mov %rsp, %rbp
    movq $0, list_node_ptr
    mov $10, %rdi
    call add_node
    mov $20, %rdi
    call add_node
    mov $30, %rdi
    call add_node
    call print_list
    mov %rbp,%rsp
    pop %rbp
    ret
add_node:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    mov %rdi, -16(%rbp)
    mov $16, %rdi
    call malloc
    mov %rdi, %rax
    mov %rdi, -8(%rbp)
    mov %rax, %rbx
    mov -8(%rbp), %rdx
    mov -16(%rbp), %rax
    mov %rax, 0(%rbx)
    mov list_node_ptr, %rdx
    mov %rdx, 8(%rbx)
    mov %rbx, list_node_ptr
    mov %rbp, %rsp
    pop %rbp
    ret
print_list:
    push %rbp
    mov %rsp, %rbp
    mov list_node_ptr, %rbx
.print_list_loop:
    cmp $0, %rbx
    je print_list_end
    mov 0(%rbx), %rsi      
    lea fmt(%rip), %rdi    
    mov $0, %rax           
    call printf
    mov 8(%rbx), %rbx      
    jmp .print_list_loop
print_list_end:
    mov %rbp, %rsp
    pop %rbp
    ret


.section .note.GNU-stack,"",@progbits

