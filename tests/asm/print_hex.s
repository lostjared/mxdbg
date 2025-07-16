.section .data
    open_mode: .asciz "rb"
    error_msg: .asciz "Error has occoured: "
    error_arg_msg: .asciz "Incorrect arguments, requires 1 argument.\n"
    error_open_msg: .asciz "Error opening file.\n"
    hex_fmt:        .asciz "%02x "
    space_fmt:      .asciz "   "
    ascii_bar_fmt:  .asciz " |"
    ascii_fmt:      .asciz "%c"
    dot_fmt:        .asciz "."
    ascii_end_fmt:  .asciz "|\n"
    .extern stderr
    .extern stdout
.section .bss
    .lcomm file_handle,   8
.section .text
    .global main
    .extern exit, fprintf, fopen,gggbgb fclose, fread
main:
    push %rbp
    mov %rsp, %rbp
    subq $16, %rsp                
    cmp $2, %rdi
    jne .error_args

    mov %rsi, %rax
    mov 8(%rax), %rdi
    lea open_mode(%rip), %rsi
    call fopen
    cmpq $0, %rax
    je .error_open
    movq %rax, file_handle(%rip)

.read_loop:
    lea (%rsp), %rdi              
    movl $1, %esi                 
    movl $16, %edx
    movq file_handle(%rip), %rcx  
    call fread                    
    cmp $0, %rax
    je .done                      

    movq %rax, %r12
    movq $0, %rbx

 .print_hex_loop:
    cmpq %rbx, %r12           
    jbe .pad_hex_loop          

    movq stdout(%rip), %rdi   
    lea hex_fmt(%rip), %rsi
    movzbq (%rsp,%rbx,1), %rdx
    xor %rax, %rax
    call fprintf

    incq %rbx
    jmp .print_hex_loop

.pad_hex_loop:
    cmpq $16, %rbx 
    je .print_ascii

    movq stdout(%rip), %rdi
    lea space_fmt(%rip), %rsi
    xor %rax, %rax
    call fprintf

    incq %rbx
    jmp .pad_hex_loop

.print_ascii:
    movq $0, %rbx

    movq stdout(%rip), %rdi   
    lea ascii_bar_fmt(%rip), %rsi
    xor %rax, %rax
    call fprintf

.print_ascii_loop:
    cmpq %rbx, %r12
    jbe .end_ascii

    movzbq (%rsp,%rbx,1), %rdx
    cmpb $32, %dl
    jb .non_printable
    cmpb $126, %dl
    ja .non_printable

    movq stdout(%rip), %rdi   
    lea ascii_fmt(%rip), %rsi
    xor %rax, %rax
    call fprintf
    jmp .next_ascii

.non_printable:
    movq stdout(%rip), %rdi   
    lea dot_fmt(%rip), %rsi
    xor %rax, %rax
    call fprintf

.next_ascii:
    incq %rbx
    jmp .print_ascii_loop

.end_ascii:
    movq stdout(%rip), %rdi   
    lea ascii_end_fmt(%rip), %rsi
    xor %rax, %rax
    call fprintf
    jmp .read_loop
.done:
    movq file_handle(%rip), %rdi
    call fclose

.exit_main:
    movl $0, %eax
    mov %rbp, %rsp
    pop %rbp
    ret
.error_args:
    movq stderr(%rip), %rdi
    lea error_arg_msg(%rip), %rsi
    xor %rax, %rax
    call fprintf
    mov $1, %rdi
    call exit
    mov %rbp, %rsp
    pop %rbp
    ret
.error_open:
    movq stderr(%rip), %rdi
    lea error_open_msg(%rip), %rsi
    xor %rax, %rax
    call fprintf
    mov $1, %rdi
    call exit
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits

