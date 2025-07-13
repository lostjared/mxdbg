.section .data
    window_title: .asciz "Pixels"
    background_path: .asciz "bg.bmp"
    error_msg: .asciz "Error: %s\n"
    bpp_msg: .asciz "Bits Per Pixel: %d\n"
    ptr_msg: .asciz "Ptr value: %p\n"
    num_msg: .asciz "Hex value: %x\n"
.section .bss
    .lcomm window_ptr, 8      
    .lcomm event_buffer, 64  
    .lcomm surface_ptr, 8
    .lcomm offscreen_surface, 8
    .extern stderr
.section .text
    .extern SDL_Init
    .extern SDL_CreateWindow
    .extern SDL_PollEvent
    .extern SDL_DestroyWindow
    .extern SDL_Quit
    .extern SDL_Delay
    .extern SDL_LockSurface
    .extern SDL_UnlockSurface
    .extern SDL_FreeSurface
    .extern SDL_UpperBlit
    .extern SDL_Delay
    .extern fprintf
    .extern srand
    .extern time
    .extern exit
    .extern SDL_GetError
    .extern RandomPixels, CreateSurface, PrintError
    .global main
    .global print_integer
main:
    push %rbp
    mov %rsp, %rbp
    sub $64, %rsp
    movq $0, %rdi         
    call time
    movq %rax, %rdi
    call srand
    movl $0x20, %edi
    call SDL_Init
    testl %eax, %eax
    jnz exit_error
    movq $window_title, %rdi
    movl $100, %esi      
    movl $100, %edx      
    movl $640, %ecx      
    movl $480, %r8d      
    movl $0, %r9d        
    call SDL_CreateWindow
    testq %rax, %rax
    jz cleanup_and_exit
    movq %rax, window_ptr(%rip)
    movq window_ptr(%rip), %rdi     
    call SDL_GetWindowSurface
    testq %rax,%rax
    jz error_exit
    movq %rax,surface_ptr(%rip)
    movq surface_ptr(%rip), %r12
    movq stderr(%rip), %rdi
    lea ptr_msg(%rip), %rsi
    movq %r12, %rdx
    call fprintf
    mov $640, %rdi
    mov $480, %rsi
    call CreateSurface
    movq %rax, offscreen_surface(%rip)
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
    movl event_buffer+20, %eax
    jmp main_loop
render_frame:
    movq offscreen_surface(%rip), %rdi
    call RandomPixels
    movq offscreen_surface(%rip), %rdi   # src surface
    movq $0, %rsi                        # src rect (NULL)
    movq surface_ptr(%rip), %rdx         # dst surface
    movq $0, %rcx                        # dst rect (NULL)
    call SDL_UpperBlit
    movq surface_ptr(%rip), %rdi

    movq window_ptr(%rip),%rdi
    call SDL_UpdateWindowSurface
    mov $1, %rdi
    call SDL_Delay
    jmp main_loop
cleanup_all:
cleanup_window:
    movq window_ptr(%rip), %rdi
    call SDL_DestroyWindow
    movq offscreen_surface(%rip), %rdi
    call SDL_FreeSurface
cleanup_and_exit:
    call SDL_Quit
    mov $0, %edi
    call exit
error_exit:
exit_error:
    call SDL_GetError
    mov %rax, %rdx
    mov stderr(%rip), %rdi
    lea error_msg(%rip), %rsi
    call fprintf
    call SDL_Quit
    movl $1, %edi
    call exit
print_integer:
     push %rbp
     mov %rsp, %rbp
     movl %edi, %edx
     movq stderr(%rip), %rdi
     lea num_msg(%rip), %rsi
     call fprintf
     mov %rbp, %rsp
     pop %rbp
     ret


.section .note.GNU-stack, "",@progbits

