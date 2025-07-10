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
    .global DrawBlocks
    .global InitBlocks
InitBlocks:
    push %rbp
    mov %rsp, %rbp
    movl $0, y_cord(%rip)
    movl $8, x_cord(%rip)
    call rand_mod5
    movl %eax, colors+8(%rip)
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
    xor %r14, %r14
    xor %r15, %r15
draw_loop:
    lea colors(%rip),%r9
    movl  (%r9,%r15,4),%edi
    call  Color
    movq renderer_ptr(%rip), %rdi
    movl rval(%rip), %esi
    movl gval(%rip), %edx
    movl bval(%rip), %ecx
    movl $255, %r8d
    call SDL_SetRenderDrawColor
    movq renderer_ptr(%rip), %rdi
    movl x_cord(%rip),%eax
    imul $34,%eax,%esi     
    addl $16,%esi
    movl y_cord(%rip),%eax
    addl %r15d,%eax
    imul $18,%eax,%edx
    addl $16,%edx
    movl $32,%ecx
    movl $16,%r8d
    call Rectangle
    incl %r15d
    cmpl $3,%r15d
    jl draw_loop
    mov %rbp, %rsp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits


