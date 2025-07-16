
.section .data


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
    movq $0, list_node_ptr(%rip)
    mov $10, %rdi
    call add_node
    mov $20, %rdi
    call add_node
    mov $30, %rdi
    call add_node
    call print_list
    call free_list
    mov $0, %rax
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
    mov %rax, %rbx
    mov -16(%rbp), %rax
    mov %rax, 0(%rbx)
    movq list_node_ptr(%rip), %rdx
    mov %rdx, 8(%rbx)
    movq %rbx, list_node_ptr(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret
print_list:
    push %rbp
    mov %rsp, %rbp
    movq list_node_ptr(%rip), %rbx
.print_list_loop:
    cmp $0, %rbx
    je .print_list_end
    mov 0(%rbx), %rsi      
    lea fmt(%rip), %rdi    
    mov $0, %rax           
    call printf
    mov 8(%rbx), %rbx      
    jmp .print_list_loop
.print_list_end:
    mov %rbp, %rsp
    pop %rbp
    ret
free_list:
    push %rbp
    mov %rsp, %rbp
    movq list_node_ptr(%rip), %rbx
.free_list_loop:
    cmp $0, %rbx
    je .free_list_end
    mov 8(%rbx), %rcx
    mov %rbx, %rdi
    push %rcx
    call free
    pop %rcx
    mov %rcx, %rbx
    jmp .free_list_loop
.free_list_end:
    movq $0, list_node_ptr(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

