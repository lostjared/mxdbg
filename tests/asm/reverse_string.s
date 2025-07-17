.section .data

.section .rodata
    format_ch: .asciz "%c"
    nl: .asciz "\n"
    input_str: .asciz "Input String: "
    buffer_len: .long 255
    .extern stdin

.section .bss
    .lcomm  buffer, 256

.section .text

    .global main
    .extern printf, fgets, strlen

main:
    push %rbp
    mov %rsp, %rbp
    lea input_str(%rip), %rdi
    xor %rax, %rax
    call printf
    call input_string
    lea buffer(%rip), %rdi
    call strlen
    mov %rax, %rsi
    lea buffer(%rip), %rdi
    call reverse_string
    lea buffer(%rip), %rdi
    call strlen
    cmp $0, %rax
    je .skip_nl
    lea nl(%rip), %rdi
    xor %rax, %rax
    call printf
.skip_nl:
    mov %rbp, %rsp
    pop %rbp
    ret
#rdi - string
#rsi - length
reverse_string:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rdi, %rcx
    movq %rcx, -8(%rbp)
    add %rsi, %rcx
    dec %rcx
.print_loop:
    mov -8(%rbp), %rdi
    cmp %rcx, %rdi # rdi > %rcx 
    jg .over
    movzx (%rcx), %eax
    movl %eax, -12(%rbp) 
    lea format_ch(%rip), %rdi
    movl -12(%rbp), %esi
    push %rcx
    xor %rax, %rax
    call printf
    pop %rcx
    dec %rcx
    jmp .print_loop
.over:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret

input_string:
    push %rbp
    mov %rsp, %rbp
    lea buffer(%rip), %rdi
    movl buffer_len(%rip), %esi
    movq stdin(%rip), %rdx
    call fgets
    lea buffer(%rip), %rdi
    call strlen 
    lea buffer(%rip), %rdi
    cmp $0, %rax
    je .input_
    add %rax, %rdi
    dec %rdi
    movb $0, (%rdi)
.input_:
    mov $0, %rax
    mov %rbp, %rsp
    pop %rbp
    ret


.section .note.GNU-stack,"",@progbits



