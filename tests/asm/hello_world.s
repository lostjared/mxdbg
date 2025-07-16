.section .data
    hello_msg: .asciz "Hello, World!\n"
    
.section .text
    .global _start

_start:
    call main
    # Exit program
    mov $60, %rax       # sys_exit
    mov $0, %rdi        # exit status
    syscall
main:
    # Write system call
    movl $0, %ebx
loop_start:
    mov $1, %rax        # sys_write
    mov $1, %rdi        # stdout
    lea hello_msg(%rip), %rsi
    mov $14, %rdx # message length
    syscall
    incl %ebx
    cmpl $10, %ebx
    jne loop_start
    mov $0, %rax
    ret
