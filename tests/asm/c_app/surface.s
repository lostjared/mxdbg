.section .data
    surf_error_msg: .asciz "SDL_Surface: Error Occoured: %s\n"
    .extern stderr
.section .bss

.section .text
    .extern SDL_CreateRGBSurface
    .extern fprintf, exit
    .global CreateSurface, PrintError
    .global SetPixel
    .extern rand, SDL_GetError
CreateSurface:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
    mov %rsi, %rdx
    mov %rdi, %rsi
    xor %edi, %edi
    mov $32, %ecx
    mov $0x00FF0000, %r8d
    mov $0x0000FF00, %r9d
    mov $0x000000FF, %r10d
    mov $0xFF000000, %r11d
    mov %r10d, %eax
    mov %rax, (%rsp)
    mov %r11d, %eax
    mov %rax, 8(%rsp)
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
# void SetPixel(void *buffer, int x, int y, unsigned int pitch, unsigned int color);
SetPixel:
    push %rbp
    mov %rsp, %rbp
    push %rax
    movslq %edx, %rax
    imulq  %rcx, %rax
    movslq %esi, %rdx
    shl $2, %rdx
    addq %rdx, %rax
    addq %rdi, %rax 
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
    xor %edx, %edx
    divl %ecx
    mov %edx, %eax
    pop %rbp
    ret

.section .note.GNU-stack, "",@progbits

