.section .data
    head: .quad 0                    # Pointer to head of linked list
    
    # Proper node structure: each node has [data][next] as consecutive memory
    node1: .quad 10                  # node1 data
           .quad node2               # node1 next pointer
    
    node2: .quad 20                  # node2 data  
           .quad node3               # node2 next pointer
    
    node3: .quad 30                  # node3 data
           .quad 0                   # node3 next pointer (NULL)
    
    # Strings for output
    node_msg: .ascii "Node data: "
    node_msg_len = . - node_msg
    
    newline: .ascii "\n"
    newline_len = . - newline
    
    # Buffer for number conversion
    number_buffer: .space 20

.section .text
    .globl _start

_start:
    # Set head to point to first node
    movq $node1, %rax
    movq %rax, head
    
    # Traverse and print the list
    movq head, %rdi                 # Start at head
    
traverse_loop:
    cmpq $0, %rdi                   # Check if current node is NULL
    je end_program                  # If NULL, end traversal
    
    # Save current node pointer
    pushq %rdi
    
    # Print "Node data: " message
    movq $1, %rax                   # sys_write
    movq $1, %rdi                   # stdout
    movq $node_msg, %rsi            # message buffer
    movq $node_msg_len, %rdx        # message length
    syscall
    
    # Restore current node pointer
    popq %rdi
    
    # Print the data from current node
    movq (%rdi), %rax               # Load data from current node
    pushq %rdi                      # Save node pointer again
    call print_number
    call print_newline
    popq %rdi                       # Restore node pointer
    
    # Move to next node
    movq 8(%rdi), %rdi              # Load next pointer (offset 8 bytes)
    jmp traverse_loop

# Function to print a number in %rax
print_number:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rdi
    pushq %rsi
    
    # Convert number to string
    movq %rax, %rbx                 # Save number
    movq $number_buffer + 19, %rdi  # Point to end of buffer
    movb $0, (%rdi)                 # Null terminate
    decq %rdi
    
    # Handle zero case
    cmpq $0, %rbx
    jne convert_loop
    movb $'0', (%rdi)
    jmp print_string
    
convert_loop:
    cmpq $0, %rbx
    je print_string
    
    xorq %rdx, %rdx                 # Clear remainder
    movq %rbx, %rax                 # Move number to rax for division
    movq $10, %rcx                  # Divisor
    divq %rcx                       # rax = rax/10, rdx = remainder
    
    addb $'0', %dl                  # Convert digit to ASCII
    movb %dl, (%rdi)                # Store digit
    decq %rdi                       # Move backward in buffer
    
    movq %rax, %rbx                 # Update number
    jmp convert_loop

print_string:
    incq %rdi                       # Point to first character
    movq $number_buffer + 20, %rcx  # End of buffer
    subq %rdi, %rcx                 # Calculate length
    
    # Save the string pointer from %rdi to %rsi before overwriting %rdi
    movq %rdi, %rsi                 # Save buffer pointer to %rsi
    movq $1, %rax                   # sys_write
    movq $1, %rdi                   # stdout
    # %rsi already contains the buffer pointer
    movq %rcx, %rdx                 # string length
    syscall
    
    popq %rsi
    popq %rdi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rbp
    ret

# Function to print newline
print_newline:
    pushq %rax
    pushq %rdi
    pushq %rsi
    pushq %rdx
    
    movq $1, %rax                   # sys_write
    movq $1, %rdi                   # stdout
    movq $newline, %rsi             # newline character
    movq $newline_len, %rdx         # length
    syscall
    
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rax
    ret

end_program:
    # Exit program
    movq $60, %rax                  # sys_exit
    movq $0, %rdi                   # exit status
    syscall

.section .note.GNU-stack,"",@progbits

