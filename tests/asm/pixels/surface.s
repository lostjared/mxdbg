.section .data
    surf_error_msg: .asciz "SDL_Surface: Error Occoured: %s\n"
    .extern stderr
.section .bss

.section .text
    .extern SDL_CreateRGBSurface
    .extern fprintf, exit, print_integer
    .global CreateSurface, PrintError
    .global RandomPixels
    .extern SDL_LockSurface
    .extern SDL_UnlockSurface
    .extern SDL_MapRGBA
    .extern rand, SDL_GetError
    .global RandomPixels, CreateSurface


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
    test %rax, %rax
    jz PrintError
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
    push %r12
    push %rbx
    sub $32, %rsp

    mov %rdi, %r14
    mov %r14, %rdi
    call SDL_LockSurface
    test %eax, %eax
    jnz PrintError

    
    mov 0x10(%r14), %r11d  
    mov 0x14(%r14), %r10d  
    mov 0x20(%r14), %r15   
    mov 0x18(%r14), %ebx   

    xor %r12d, %r12d       
.y_loop:
    xor %r13d, %r13d       
.x_loop:
    call rand_mod255
    mov %eax, -12(%rbp)         
    call rand_mod255
    mov %eax, -16(%rbp)
    call rand_mod255
    mov %eax, -20(%rbp)          
    mov 8(%r14), %rdi      
    mov $0xff, %r8d
    mov -12(%rbp), %esi
    mov -16(%rbp), %edx
    mov -20(%rbp), %rcx
    call SDL_MapRGBA       
    mov %eax, %r9d
    
    mov %r15, %rdi      
    mov %r12d, %esi     
    mov %r13d, %edx     
    mov %ebx, %ecx      
    mov %r9d, %r8d      
    call SetPixel

    inc %r13d
    cmp %r11d, %r13d
    jl .x_loop

    inc %r12d
    cmp %r10d, %r12d
    jl .y_loop

.unlock:
    mov %r14, %rdi
    call SDL_UnlockSurface
.cleanup:
    add $32, %rsp
    pop %rbx
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    pop %rbp
    ret

SetPixel:
    push %rbp
    mov %rsp, %rbp
    push %rax          
    movslq %esi, %rax  
    imulq %rcx, %rax   
    addq %rdi, %rax    
    movslq %edx, %rdx
    shl $2, %rdx
    addq %rdx, %rax
    mov %r8d, (%rax)
    pop %rax
    mov %rbp, %rsp
    pop %rbp
    ret

rand_mod255:
    push %rbp
    mov %rsp, %rbp
    call rand          
    mov $255, %ecx
    cdq                
    idivl %ecx         
    mov %edx, %eax
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits


