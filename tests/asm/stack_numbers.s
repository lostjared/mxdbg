.section .data
    stack_size: .quad 255
    .extern stdout, stderr

.section .rodata
    overflow_msg: .asciz "Stack overflow.\n"
    underflow_msg: .asciz "Stack underflow.\n"
    stack_output: .asciz "Stack [%ld] -> %ld\n"
    stack_pop_output: .asciz "Stack [%ld] <- %ld\n"
    stack_elements: .asciz "Stack Elements: %ld\n"
.section .bss
    .lcomm stack_data, 2040
    .lcomm stack_index, 8
.section .text
    .global main
    .extern puts, fprintf, exit, printf

main:
    push %rbp
    mov %rsp, %rbp
    call init_stack
    mov $1, %rdi
    call push_stack
    mov $5, %rdi
    call push_stack
    mov $6, %rdi
    call push_stack
    call print_stack
    call pop_stack
    call pop_stack
    call pop_stack
    mov %rbp, %rsp
    pop %rbp
    ret

init_stack:
    push %rbp
    mov %rsp, %rbp
    movq $0, stack_index(%rip)   
    mov %rbp, %rsp
    pop %rbp
    ret

# in rdi
push_stack:
    push %rbp
    mov %rsp, %rbp
    lea stack_data(%rip), %rax
    movq stack_index(%rip), %rcx
    movq stack_size(%rip), %rbx
    cmp %rbx, %rcx
    jae .s_overflow
    movq %rcx, %r8
    imul $8, %r8
    addq %r8, %rax
    movq %rdi, (%rax)
    incq %rcx
    movq %rcx, stack_index(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret
.s_overflow:
    lea overflow_msg(%rip), %rsi
    movq stderr(%rip), %rdi
    call fprintf
    mov $1, %rdi
    call exit

pop_stack:
    push %rbp
    mov %rsp, %rbp
    mov stack_index(%rip), %rax
    dec %rax
    cmp $-1, %rax
    je .underflow
    lea stack_data(%rip), %rcx
    mov %rax, %r8
    movq %rax, stack_index(%rip)
    imul $8, %rax
    addq %rax, %rcx
    movq (%rcx), %rdx
    movq %r8, %rsi
    lea stack_pop_output(%rip), %rdi
    xor %rax, %rax
    call printf
    mov %rdx, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

.underflow:
    movq stderr(%rip), %rdi
    lea underflow_msg(%rip), %rsi
    call fprintf
    mov $1, %rdi
    call exit

print_stack:
    push %rbp
    mov %rsp, %rbp
    lea stack_elements(%rip), %rdi
    movq stack_index(%rip), %rsi
    call printf
    lea stack_data(%rip), %rax
    movq stack_index(%rip), %rcx
    movq $0, %rdi
.print_loop:
    cmpq %rdi, %rcx
    jle .exit_loop
    movq %rdi, %rdx
    imul $8, %rdx
    lea stack_data(%rip), %rax
    addq %rdx, %rax
    mov %rdi, %rsi
    push %rdi
    lea stack_output(%rip), %rdi
    movq (%rax), %rdx
    xor %rax, %rax
    call printf
    pop %rdi
    incq %rdi
    movq stack_index(%rip), %rcx
    jmp .print_loop
.exit_loop:
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits



