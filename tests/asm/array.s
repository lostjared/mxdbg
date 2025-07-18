.section .data
    prompt_msg: .asciz "Enter 5 numbers:\n"
    input_prompt: .asciz "Enter number %d: "
    output_format: .asciz "Number %d: %d\n"
    scan_format: .asciz "%d"
    
    .align 4
    array: .space 20     # Reserve space for 5 integers (4 bytes each)

.section .text
.globl main

main:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    sub $8, %rsp           # Align stack for calls

    # Display initial prompt
    movq $prompt_msg, %rdi
    xorl %eax, %eax
    call printf

    # Initialize counter for input loop
    movl $0, %r12d      # i = 0
    
input_loop:
    # Check if we've read 5 numbers
    cmpl $5, %r12d
    jge print_array
    
    # Print the input prompt with current number
    movq $input_prompt, %rdi
    movl %r12d, %esi
    addl $1, %esi       # Add 1 to display 1-based index
    xorl %eax, %eax
    call printf
    
    # Read a number
    movq $scan_format, %rdi
    leaq array(,%r12,4), %rsi   # Calculate address of array[i]
    xorl %eax, %eax
    call scanf
    
    # Increment counter
    incl %r12d
    jmp input_loop

print_array:
    # Print header for output
    movq $prompt_msg, %rdi    # Reuse the prompt message as a header
    xorl %eax, %eax
    call printf
    
    # Initialize counter for output loop
    movl $0, %r12d      # i = 0
    
output_loop:
    # Check if we've printed all 5 numbers
    cmpl $5, %r12d
    jge exit
    
    # Print current array element
    movq $output_format, %rdi
    movl %r12d, %esi
    addl $1, %esi       # Add 1 to display 1-based index
    movl array(,%r12,4), %edx   # Load value of array[i]
    xorl %eax, %eax
    call printf
    
    # Increment counter
    incl %r12d
    jmp output_loop

exit:
    # Clean up and return
    mov $0, %eax
    add $8, %rsp           # Deallocate alignment space
    pop %r13
    pop %r12
    leave
    ret

.section .note.GNU-stack,"",@progbits
