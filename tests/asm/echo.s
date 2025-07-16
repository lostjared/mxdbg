.section .data
    invalid_args_msg: .asciz "Error invalid arguments. \nRequires two arguments: %s <file> <timeout>\n"
    file_error_msg: .asciz "Error could not open file: %s\n"
    timeout_err_msg: .asciz "Error invalid timeout...\n"
    file_mode: .asciz "r"
    output_byte: .asciz "%c"
    .extern stderr, stdout
.section .bss
    .lcomm byte_value, 1
.section .text
    .global main
    .extern fopen, fclose, fread, atoi, usleep, fflush
main:
    push %rbp
    mov %rsp, %rbp
    sub $64, %rsp
    mov (%rsi), %rdx
    cmp $3, %rdi
    jne .invalid_args
    movq 8(%rsi), %rax
    mov %rax, -8(%rbp)
    movq 16(%rsi), %rax
    mov  %rax, -16(%rbp)
    mov -8(%rbp), %rdi
    lea file_mode(%rip), %rsi
    call fopen
    mov -8(%rbp), %rdx
    cmp $0, %rax
    je .file_open_error
    mov %rax, -24(%rbp)
    mov -16(%rbp), %rdi
    call atoi
    cmp $0, %rax
    jle .timeout_error
    imul $1000, %rax, %rax
    mov %rax, -32(%rbp)
.loop1:
    mov -24(%rbp), %rcx
    lea byte_value(%rip), %rdi
    mov $1, %rsi
    mov $1, %rdx
    call fread
    cmp $0, %rax
    je .over
    lea output_byte(%rip), %rdi
    movzx byte_value(%rip), %esi
    xor %rax, %rax
    call printf
    movq stdout(%rip), %rdi
    call fflush 
    mov -32(%rbp), %rdi
    call usleep
    jmp .loop1
.over:
    mov -24(%rbp), %rdi
    call fclose
    mov %rbp, %rsp
    pop %rbp
    ret
.invalid_args:
    movq stderr(%rip), %rdi
    lea invalid_args_msg(%rip), %rsi
    xor %rax, %rax
    call fprintf
    mov $1, %rdi
    call exit  
.file_open_error:
    movq stderr(%rip), %rdi
    lea file_error_msg(%rip), %rsi
    xor %rax, %rax
    call fprintf
    mov $1, %rdi
    call exit
.timeout_error:
    movq stderr(%rip), %rdi
    lea timeout_err_msg(%rip), %rsi
    xor %rax, %rax
    call fprintf
    mov $1, %rdi
    call exit

.section .note.GNU-stack,"",@progbits

