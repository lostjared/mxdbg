.section .data
    window_title: .asciz "Hello, World, SDL2 Assembly"
.section .bss
    .lcomm window_ptr, 8      
    .lcomm renderer_ptr, 8    
    .lcomm event_buffer, 64   
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
    .extern srand
    .extern time
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $16, %rsp
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
    movq %rax, window_ptr
    movq window_ptr, %rdi     
    movl $-1, %esi            
    movl $0, %edx             
    call SDL_CreateRenderer
    testq %rax, %rax
    jz cleanup_window
    movq %rax, renderer_ptr
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
render_frame:
    movq renderer_ptr, %rdi
    movl $0, %esi             
    movl $0, %edx           
    movl $0, %ecx           
    movl $255, %r8d           
    call SDL_SetRenderDrawColor
    movq renderer_ptr, %rdi
    call SDL_RenderClear
    movq renderer_ptr, %rdi
    call DrawGrid
    movq renderer_ptr, %rdi
    call SDL_RenderPresent
    movl $16, %edi
    call SDL_Delay
    jmp main_loop
cleanup_all:
    movq renderer_ptr, %rdi
    call SDL_DestroyRenderer
cleanup_window:
    movq window_ptr, %rdi
    call SDL_DestroyWindow
cleanup_and_exit:
    call SDL_Quit
    mov $0, %edi
    call exit
exit_error:
    call SDL_Quit
    movl $1, %edi
    call exit

.section .note.GNU-stack, "",@progbits
