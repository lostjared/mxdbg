.section .data
    input_message: .asciz "Enter a string: \n"
    output_message: .asciz "The string is: %s\n"
    added_string: .asciz " added to end of string\n"

.section .bss
    input_buffer: .space 256              # Reserve 256 bytes, zero-initialized

.section .text
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp                     # Align stack to 16 bytes
    
    # Print input prompt
    lea input_message(%rip), %rdi
    mov $0, %rax                      # No vector registers used
    call printf
    
    # Read input
    mov $0, %rax                      # sys_read
    mov $0, %rdi                      # stdin
    lea input_buffer(%rip), %rsi      # buffer
    mov $255, %rdx                    # max bytes (leave room for null)
    syscall
    
    # Check if we read anything
    test %rax, %rax
    jle error_exit
    
    # Ensure null termination and remove newline
    mov %rax, %rcx                    # rcx = bytes read
    lea input_buffer(%rip), %rsi      # rsi = input_buffer
    add %rcx, %rsi                    # rsi = input_buffer + bytes_read
    movb $0, (%rsi)                   # null terminate at end
    # Check if previous character is newline and remove it
    dec %rsi                          # go back one position
    cmpb $10, (%rsi)                  # check if it's newline (ASCII 10)
    jne no_newline
    movb $0, (%rsi)                   # replace newline with null
    
no_newline:
    # Get string length
    lea input_buffer(%rip), %rdi      # rdi for strlen parameter
    call strlen
    mov %rax, %r8                     # save strlen result
    
    # Append additional string
    lea input_buffer(%rip), %rsi      # rsi = address of input_buffer
    add %r8, %rsi                     # rsi = end of input string
    lea added_string(%rip), %rdi      # rdi = address of added_string

copy_loop:
    movb (%rdi), %al                  # load byte from added_string
    movb %al, (%rsi)                  # store byte to end of input_buffer
    test %al, %al                     # check for null terminator
    jz copy_done                      # exit if null
    inc %rsi
    inc %rdi
    jmp copy_loop
    
copy_done:
    # Print result
    lea output_message(%rip), %rdi
    lea input_buffer(%rip), %rsi
    mov $0, %rax                      # No vector registers used
    call printf
    
    # Exit
    mov $0, %rax                      # exit code 0
    mov %rbp, %rsp
    pop %rbp
    ret

error_exit:
    mov $1, %rax                      # exit code 1
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

