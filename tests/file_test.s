# for testing the Debugger
# This assembly code creates a file named "test.txt" and writes "Hello, World" to it.
# It uses the x86-64 Linux syscall interface.

.section .data
    filename:   .string "test.txt"
    message:    .asciz "Hello, World\n"
    message_len = . - message

.section .text
    .global _start

_start:
    # Open file (create if it doesn't exist, truncate if it does)
    mov $2, %rax           # sys_open syscall number
    mov $filename, %rdi    # filename
    mov $0x241, %rsi       # O_CREAT | O_WRONLY | O_TRUNC (64 | 1 | 512 = 577 = 0x241)
    mov $0644, %rdx        # File permissions (rw-r--r--)
    syscall
    
    # Save file descriptor
    mov %rax, %r8
    
    # Write to file
    mov $1, %rax           # sys_write syscall number
    mov %r8, %rdi          # file descriptor
    mov $message, %rsi     # message to write
    mov $message_len, %rdx # message length
    dec %rdx               # Adjust length for syscall
    syscall
    
    # Close file
    mov $3, %rax           # sys_close syscall number
    mov %r8, %rdi          # file descriptor
    syscall
    
    # Exit
    mov $60, %rax          # sys_exit syscall number
    xor %rdi, %rdi         # exit code 0
    syscall

.section .note.GNU-stack,"",@progbits
