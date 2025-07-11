.section .data
    font_name: .asciz "font.ttf"
    font_error: .asciz "Error loading font"
    render_error: .asciz "Render Text Failred"
    font_loaded: .asciz "Font loaded."
.section .bss
    .lcomm font_ptr, 8
    .comm color_r, 1
    .comm color_g, 1
    .comm color_b, 1
    .comm color_a, 1
    .globl color_r
    .globl color_g
    .globl color_b
    .globl color_a

.section .text
    .extern TTF_Init
    .extern TTF_Quit
    .extern TTF_RenderText_Solid
    .extern SDL_CreateTextureFromSurface
    .extern SDL_FreeSurface
    .extern SDL_DestroyTexture
    .extern exit
    .global printtext
    .global init_text
    .global quit_text
# edi font size
init_text:
    push %rbp   
    mov %rsp, %rbp
    movl %edi, %r13d
    call TTF_Init
    movl %r13d, %esi
    lea font_name(%rip), %rdi
    call TTF_OpenFont
    movq %rax, font_ptr(%rip)
    cmp $0, %rax
    je error_init_exit
    lea font_loaded(%rip), %rdi
    call puts
    movb $255, color_r(%rip)
    movb $255, color_g(%rip)
    movb $255, color_b(%rip)
    mov %rbp, %rsp
    pop %rbp
    ret
error_init_exit:
    lea font_error(%rip), %rdi
    call puts
    mov $1, %rdi
    call exit
    ret

quit_text:
    push %rbp
    mov %rsp, %rbp
    movq font_ptr(%rip), %rdi
    call TTF_CloseFont
    call TTF_Quit
    mov %rbp, %rsp
    pop %rbp
    ret

#rdi, renderer
#rsi, string
#edx, x
#ecx, y
printtext:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    sub $0x40, %rsp

    mov %rdi, %r12
    mov %rsi, %r13
    mov %edx, %r14d
    mov %ecx, %r15d

    movb color_r(%rip), %al
    movb %al, 28(%rsp)
    movb color_g(%rip), %al
    movb %al, 29(%rsp)
    movb color_b(%rip), %al
    movb %al, 30(%rsp)
    movb $255, %al
    movb %al, 31(%rsp)

    movq font_ptr(%rip), %rdi
    movq %r13, %rsi
    movl 28(%rsp), %edx
    call TTF_RenderText_Solid
    mov %rax, %r13
    cmp $0, %rax
    je render_fail

    mov %r12, %rdi
    mov %r13, %rsi
    call SDL_CreateTextureFromSurface
    mov %rax, %r12
    cmp $0, %rax
    je render_fail

    movl %r14d, 0(%rsp) 
    movl %r15d, 4(%rsp) 
    movl 16(%r13), %eax 
    movl %eax, 8(%rsp)  
    movl 20(%r13), %eax 
    movl %eax, 12(%rsp) 

    mov %r13, %rdi
    call SDL_FreeSurface

    movq renderer_ptr(%rip), %rdi
    mov %r12, %rsi
    mov $0, %rdx   # srcrect = NULL
    lea 0(%rsp), %rcx # &dstrect
    call SDL_RenderCopy

    mov %r12, %rdi
    call SDL_DestroyTexture
    add $0x40, %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    mov %rbp, %rsp
    pop %rbp
    ret

render_fail:
    lea render_error(%rip), %rdi
    call puts
    mov $1, %rdi
    call exit



.section .note.GNU-stack, "",@progbits

