.section .data
    window_title: .asciz "Assembly - SDL Font Load"
    background_path: .asciz "bg.bmp"
    error_msg: .asciz "Error loading background image\n"
    open_mode: .asciz "rb"
    hello_world: .asciz "Hello, World in x64 ASM!"
.section .bss
    .lcomm window_ptr, 8      
    .lcomm bg_surface, 8
    .lcomm bg_texture, 8
    .comm renderer_ptr, 8    
    .lcomm event_buffer, 64  
    .globl renderer_ptr 
.section .text
    .extern SDL_Init
    .extern SDL_CreateWindow
    .extern SDL_CreateRenderer
    .extern SDL_SetRenderDrawColor
    .extern SDL_RenderClear
    .extern SDL_RenderPresent
    .extern SDL_PollEvent
    .extern SDL_DestroyRenderer
    .extern SDL_DestroyWindow
    .extern SDL_Quit
    .extern SDL_Delay
    .extern SDL_RWFromFile
    .extern SDL_LoadBMP_RW
    .extern SDL_DestroyTexture
    .extern SDL_RenderCopy
    .extern puts
    .extern exit
    .extern init_text
    .extern quit_text

    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    movl $0x20, %edi
    call SDL_Init
    testl %eax, %eax
    jnz exit_error
    movq $window_title, %rdi
    movl $100, %esi      
    movl $100, %edx      
    movl $1280, %ecx      
    movl $720, %r8d      
    movl $0, %r9d        
    call SDL_CreateWindow
    testq %rax, %rax
    jz cleanup_and_exit
    movq %rax, window_ptr(%rip)
    movq window_ptr(%rip), %rdi     
    movl $-1, %esi            
    movl $0, %edx             
    call SDL_CreateRenderer
    testq %rax, %rax
    jz cleanup_window
    movq %rax, renderer_ptr(%rip)
    lea background_path(%rip), %rdi
    lea open_mode(%rip), %rsi
    call SDL_RWFromFile
    cmp $0, %rax
    je error_exit
    mov %rax, %rdi
    movl $1, %esi
    call SDL_LoadBMP_RW
    cmp $0, %rax
    je error_exit
    movq %rax, bg_surface(%rip)
    movq bg_surface(%rip), %rsi
    movq renderer_ptr(%rip), %rdi
    call SDL_CreateTextureFromSurface
    cmp $0, %rax
    je error_exit
    movq %rax, bg_texture(%rip)
    movq bg_surface(%rip), %rdi
    call SDL_FreeSurface
    movl $32, %edi
    call init_text
main_loop:
    movq $event_buffer, %rdi
    call SDL_PollEvent
    testl %eax, %eax
    jz render_frame 
    movl event_buffer, %eax
    cmpl $0x100, %eax    
    je cleanup_all
    cmpl $0x300, %eax
    jne render_frame
    movl event_buffer+16, %eax
    cmpl $0x29, %eax          
    je cleanup_all
    jmp main_loop
render_frame:
    movq renderer_ptr(%rip), %rdi
    movl $0, %esi             
    movl $0, %edx           
    movl $0, %ecx           
    movl $255, %r8d           
    call SDL_SetRenderDrawColor
    movq renderer_ptr(%rip), %rdi
    call SDL_RenderClear
    movq renderer_ptr(%rip), %rdi
    movq bg_texture(%rip), %rsi
    mov $0, %rdx
    mov $0, %rcx
    call SDL_RenderCopy

    movq renderer_ptr(%rip), %rdi
    lea hello_world(%rip), %rsi
    movl $15, %edx
    movl $15, %ecx
    movb $255, color_r(%rip)
    movb $255, color_g(%rip)
    movb $255, color_b(%rip)
    call printtext
    movq renderer_ptr(%rip), %rdi
    call SDL_RenderPresent
    movl $4, %edi
    call SDL_Delay
    jmp main_loop
cleanup_all:
    movq bg_texture(%rip), %rdi
    call SDL_DestroyTexture
    movq renderer_ptr(%rip), %rdi
    call SDL_DestroyRenderer
    call quit_text
cleanup_window:
    movq window_ptr(%rip), %rdi
    call SDL_DestroyWindow

cleanup_and_exit:
    call SDL_Quit
    mov $0, %edi
    call exit
error_exit:
    lea error_msg(%rip), %rdi
    call puts
exit_error:
    call SDL_Quit
    movl $1, %edi
    call exit


.section .note.GNU-stack, "",@progbits
