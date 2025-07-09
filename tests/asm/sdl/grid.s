.section .data

.section .bss
    .lcomm rect1, 16
    .lcomm renderer_ptr,  8

.section .text
    .extern SDL_SetRenderDrawColor
    .extern SDL_RenderFillRect
    .global Rectangle
    .global DrawGrid
    .global rand_mod255

Rectangle:
    push %rbp
    mov %rsp, %rbp
    movl %esi, rect1(%rip)      
    movl %edx, rect1+4(%rip)    
    movl %ecx, rect1+8(%rip)    
    movl %r8d, rect1+12(%rip)   
    lea rect1(%rip), %rsi       
    call SDL_RenderFillRect
    mov %rbp, %rsp
    pop %rbp
    ret
DrawGrid:
    push %rbp
    mov %rsp, %rbp
    sub $56, %rsp
    movq %rdi, renderer_ptr(%rip)
    movq $16, -0x8(%rbp)
    movq $16, -0x10(%rbp)
grid_loop_y:
    movq $16, -0x8(%rbp)
grid_loop_x:
    call rand_mod255
    movl %eax, -0x20(%rbp)     
    call rand_mod255
    movl %eax, -0x24(%rbp)
    call rand_mod255   
    movl %eax, -0x28(%rbp)
    
    movq renderer_ptr(%rip), %rdi     
    movl -0x20(%rbp), %esi      
    movl -0x24(%rbp), %edx      
    movl -0x28(%rbp), %ecx      
    movl $255, %r8d             
    call SDL_SetRenderDrawColor
    
    movq renderer_ptr(%rip), %rdi     
    movl -0x8(%rbp), %esi       
    movl -0x10(%rbp), %edx      
    movl $32, %ecx              
    movl $16, %r8d              
    call Rectangle
    addl $34, -0x8(%rbp)        
    cmpl $620, -0x8(%rbp)       
    jl grid_loop_x
    addq $18, -0x10(%rbp)
    cmpq $460, -0x10(%rbp)
    jl grid_loop_y
done:
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
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits



