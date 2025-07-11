.section .data
    window_title: .asciz "MasterPiece Assembly"
    background_path: .asciz "bg.bmp"
    error_msg: .asciz "Error loading background image\n"
    open_mode: .asciz "rb"
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
    .extern srand
    .extern time
    .extern exit
    .extern DrawBlocks
    .extern MoveLeft
    .extern MoveRight
    .extern MoveDown
    .extern ShiftUp
    .extern CheckGrid
    .extern CheckMoveDown
    .global main
main:
    push %rbp
    mov %rsp, %rbp
    sub $32, %rsp
    movq $0, %rdi         
    call time
    movq %rax, %rdi
    call srand
    movl $-1, %edi
    call FillGrid
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
    movl $-1, %esi            
    movl $0, %edx             
    call SDL_CreateRenderer
    testq %rax, %rax
    jz cleanup_window
    movq %rax, renderer_ptr(%rip)
    call InitBlocks

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
    cmpl $0x40000050, %eax    # Left arrow
    je key_left
    cmpl $0x4000004F, %eax    # Right arrow
    je key_right
    cmpl $0x40000051, %eax
    je key_down
    cmpl $0x40000052, %eax
    je key_up
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
    call CheckGrid
    call CheckMoveDown
    movq renderer_ptr(%rip), %rdi
    call DrawGrid
    call DrawBlocks
    movq renderer_ptr(%rip), %rdi
    call SDL_RenderPresent
    call SDL_GetTicks
    mov %eax, -4(%rbp)              
    cmpb $0, -8(%rbp)               
    je set_last_ticks
    mov -8(%rbp), %edx              
    sub %edx, %eax                  
    cmpl $1200, %eax
    jb skip_move_down
    call MoveDown
    call SDL_GetTicks
    mov %eax, -8(%rbp)              
    jmp skip_move_down
set_last_ticks:
    mov -4(%rbp), %eax
    mov %eax, -8(%rbp)
skip_move_down:
    movl $4, %edi
    call SDL_Delay
    jmp main_loop
key_left:
    call MoveLeft
    jmp render_frame
key_right:
    call MoveRight
    jmp render_frame
key_down:
    call MoveDown
    jmp render_frame
key_up:
    call ShiftUp
    jmp render_frame
cleanup_all:
    movq bg_texture(%rip), %rdi
    call SDL_DestroyTexture
    movq renderer_ptr(%rip), %rdi
    call SDL_DestroyRenderer
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
