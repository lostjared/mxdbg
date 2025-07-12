.section .data
    surf_error_msg: .asciz "SDL_Surface: Error Occoured: %s\n"
    .extern stderr
.section .bss

.section .text
    .extern SDL_CreateRGBSurface
    .extern fprintf, exit, print_integer
    .global CreateSurface, PrintError
    .global RandomPixels
# width in %rdi, height in %rsi
CreateSurface:
    push %rbp
    mov %rsp, %rbp
    sub $8, %rsp
    mov %rsi, %rdx
    mov %rdi, %rsi
    mov $0, %edi
    mov $32, %ecx
    mov $0x00ff0000, %r8d 
    mov $0x0000ff00, %r9d     
    mov $0xff000000, %rax
    push %rax
    mov $0x000000ff, %rax
    push %rax
    call SDL_CreateRGBSurface
    add $16, %rsp     
    mov %rax, -0x8(%rbp)
    testl %eax, %eax
    jz PrintError
    mov -0x8(%rbp), %rax
    mov %rbp, %rsp
    pop %rbp
    ret

PrintError:
    call SDL_GetError 
    mov %rax, %rdx
    movq stderr(%rip), %rdi
    lea surf_error_msg(%rip), %rsi
    call fprintf
    mov $1, %rdi
    call exit

RandomPixels:
    push %rbp
    mov %rsp, %rbp
    push %r15
    push %r14
    push %r13        

    mov %rdi, %r14   
    
    mov %r14, %rdi
    call SDL_LockSurface
    test %eax, %eax
    jnz PrintError

    mov 8(%r14), %rax      
    mov 16(%rax), %al      
    cmp $32, %al
    jne .unlock           

.unlock:
     mov %r14, %rdi
     call SDL_UnlockSurface
.cleanup:
    pop %r13
    pop %r14
    pop %r15
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits

