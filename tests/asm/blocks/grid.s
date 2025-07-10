.section .data

.section .bss
    .lcomm rect1, 16
    .lcomm grid, 1800
    .comm rval, 4
    .comm gval, 4
    .comm bval, 4
    .extern renderer_ptr
    .globl rval
    .globl gval
    .globl bval
.section .text
    .extern SDL_SetRenderDrawColor
    .extern SDL_RenderFillRect
    .extern InitBlocks
    .extern DrawBlocks
    .global Rectangle
    .global DrawGrid
    .global FillGrid
    .global SetGrid
    .global GetGrid
    .global rand_mod5
    .global Color

FillGrid:
    push %rbp
    mov %rsp, %rbp
    push %rdi
    push %rax
    pop %rax
    pop %rdi
    movl %edi, %ebx
    movl $0, %ecx
fill_y:
    movl $0, %r9d
fill_x:
    movl %ebx, %edi
    movl %ecx, %esi
    movl %r9d, %edx
    call SetGrid
    inc %r9d    
    cmpl $18, %r9d             
    jl fill_x
    inc %ecx    
    cmpl $25, %ecx             
    jl fill_y
    mov %rbp, %rsp
    pop %rbp
    ret
Color:
    push %rbp
    mov %rsp, %rbp
    cmpl $-1, %edi
    je set_black
    cmpl $0, %edi
    je set_c1
    cmpl $1, %edi
    je set_c2
    cmpl $2, %edi
    je set_c3
    cmpl $3, %edi
    je set_c4
    cmpl $4, %edi
    je set_c5
    jmp over
set_c1:  
    movl $255, rval(%rip)
    movl $0, gval(%rip)
    movl $0, bval(%rip)
    jmp over
set_c2:
    movl $0, rval(%rip)
    movl $255, gval(%rip)
    movl $0, bval(%rip)
    jmp over
set_c3:
    movl $0, rval(%rip)
    movl $0, gval(%rip)
    movl $255, bval(%rip)
    jmp over
set_c4:
    movl $255, rval(%rip)
    movl $255, gval(%rip)
    movl $0, bval(%rip)
    jmp over
set_c5:
    movl $255, rval(%rip)
    movl $0, gval(%rip)
    movl $255, bval(%rip)
    jmp over
set_black:
    movl $0, rval(%rip)
    movl $0, gval(%rip)
    movl $0, bval(%rip)
    jmp over
over:
    mov %rbp, %rsp
    pop %rbp
    ret

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
    sub $64, %rsp
    movq %rdi, renderer_ptr(%rip)
    movl $16, -8(%rbp)
    movl $0, -24(%rbp)
grid_loop_y:
    movl $16, -16(%rbp)
    movl $0, -28(%rbp)
grid_loop_x:  
    movl -24(%rbp), %edi
    movl -28(%rbp), %esi
    call GetGrid
    movl %eax, %edi
    call Color                  
    movq renderer_ptr(%rip), %rdi
    movl rval(%rip), %esi
    movl gval(%rip), %edx
    movl bval(%rip), %ecx
    movl $255, %r8d
    call SDL_SetRenderDrawColor
    movq renderer_ptr(%rip), %rdi
    movl -16(%rbp), %esi
    movl -8(%rbp), %edx
    movl $32, %ecx
    movl $16, %r8d
    call Rectangle
nocolor:
    addl $34, -16(%rbp)
    incl -28(%rbp)
    cmpl $18, -28(%rbp)        
    jl grid_loop_x
    addl $18, -8(%rbp)
    incl -24(%rbp)
    cmpl $25, -24(%rbp)        
    jl grid_loop_y 
    mov %rbp, %rsp
    pop %rbp
    ret
rand_mod5:
    push %rbp
    mov %rsp, %rbp
    call rand          
    mov $5, %ecx
    cdq                
    idivl %ecx         
    mov %edx, %eax     
    mov %rbp, %rsp
    pop %rbp
    ret
# Get value from grid[row][col]
# Arguments: row in %edi, col in %esi
# Returns: value in %eax
GetGrid:
    push %rbp
    mov %rsp, %rbp
    lea grid(%rip), %r8
    imull $18, %edi, %eax
    addl %esi, %eax
    shll $2, %eax
    movl (%r8, %rax, 1), %ecx
    mov %ecx, %eax
    mov %rbp, %rsp
    pop %rbp
    ret
# Set grid[row][col] = value
# Arguments: value in %edi, row in %esi, col in %edx
SetGrid:
    push %rbp
    mov %rsp, %rbp
    lea grid(%rip), %r8
    imull $18, %esi, %eax
    addl %edx, %eax
    shll $2, %eax
    movl %edi, (%r8, %rax, 1)
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits



