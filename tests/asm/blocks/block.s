.section .data

.section .bss
    .lcomm colors, 12
    .comm x_cord, 4
    .comm y_cord, 4
    .extern rval    
    .extern gval
    .extern bval
    .extern renderer_ptr
    .globl x_cord
    .globl y_cord
.section .text
    .extern Rectangle
    .extern Color
    .extern SetGrid
    .extern GetGrid
    .extern rand_mod5
    .extern FillGrid
    .global DrawBlocks
    .global InitBlocks
    .global MoveLeft
    .global MoveRight
    .global MoveDown
    .global ShiftUp
InitBlocks:
    push %rbp
    mov %rsp, %rbp
    movl $0, y_cord(%rip)
    movl $8, x_cord(%rip)
    call rand_mod5
    movl %eax, colors(%rip)
    call rand_mod5
    movl %eax, colors+4(%rip)
    call rand_mod5
    movl %eax, colors+8(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret
DrawBlocks:
    push %rbp
    mov  %rsp,%rbp
    push %r12
    push %r13
    lea colors(%rip),%r13
    xor %r12d, %r12d
draw_loop:
    movl  (%r13,%r12,4),%edi
    call  Color
    movq renderer_ptr(%rip), %rdi
    movl rval(%rip), %esi
    movl gval(%rip), %edx
    movl bval(%rip), %ecx
    movl $255, %r8d
    call SDL_SetRenderDrawColor
    movq renderer_ptr(%rip), %rdi
    movl x_cord(%rip),%eax
    imul $34,%eax,%eax     
    addl $16,%eax
    movl %eax, %esi
    movl y_cord(%rip),%eax
    addl %r12d,%eax
    imul $18,%eax,%eax
    addl $16,%eax
    movl %eax, %edx
    movl $32,%ecx
    movl $16,%r8d
    call Rectangle
    incl %r12d     
    cmpl $3,%r12d
    jl draw_loop
    pop %r13
    pop %r12
    mov %rbp, %rsp
    pop %rbp
    ret

MoveLeft:
    push %rbp
    mov %rsp, %rbp    
    movl x_cord(%rip), %eax
    push %rax
    cmp $0, %eax
    je left_over
    movl %eax, %ecx
    dec %ecx
    movl y_cord(%rip), %edx
    addl $2, %edx
    movl %edx, %edi
    movl %ecx, %esi
    call GetGrid
    cmp $-1, %eax
    jne left_over
    pop %rax
    decl %eax
    movl %eax, x_cord(%rip)
left_over:
    mov %rbp, %rsp
    pop %rbp
    ret

MoveRight:
    push %rbp
    mov %rsp, %rbp
    movl x_cord(%rip), %eax
    push %rax
    cmp $17, %eax
    je right_over
    movl %eax, %ecx
    incl %ecx
    movl y_cord(%rip), %edx
    addl $2, %edx
    movl %edx, %edi
    movl %ecx, %esi
    call GetGrid
    cmpl $-1, %eax
    jne right_over
    pop %rax
    incl %eax
    movl %eax, x_cord(%rip)
right_over:
    mov %rbp, %rsp
    pop %rbp
    ret

MoveDown:
    push %rbp
    mov %rsp, %rbp
    movl y_cord(%rip), %edi
    addl $3, %edi
    movl x_cord(%rip), %esi
    call GetGrid
    cmpl $-1, %eax
    jne merge_block
    cmpl $24, %edi
    jg merge_block
    mov y_cord(%rip), %edi
    incl %edi
    mov %edi, y_cord(%rip)
down_over:
    mov %rbp, %rsp
    pop %rbp
    ret
merge_block:
    movl colors(%rip), %edi
    movl x_cord(%rip), %edx
    movl y_cord(%rip), %esi
    cmpl $0, %esi
    je clearit
    call SetGrid
    
    movl colors+4(%rip), %edi
    movl x_cord(%rip), %edx
    movl y_cord(%rip), %esi
    incl %esi
    call SetGrid

    movl colors+8(%rip), %edi
    movl x_cord(%rip), %edx
    movl y_cord(%rip), %esi
    addl $2,%esi
    call SetGrid

    call InitBlocks
    mov %rbp, %rsp
    pop %rbp
    ret
clearit:
    call InitBlocks
    movl $-1, %edi
    call FillGrid
    mov %rbp, %rsp
    pop %rbp
    ret
    
ShiftUp:
    push %rbp
    mov %rsp, %rbp
    movl colors(%rip), %eax
    movl colors+4(%rip), %ebx
    movl colors+8(%rip), %ecx
    movl %ecx, colors(%rip)
    movl %eax, colors+4(%rip)
    movl %ebx, colors+8(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits


