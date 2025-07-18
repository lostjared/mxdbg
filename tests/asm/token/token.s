.section .text
    .global tokenize

# extern char **tokenize(const char *src, const char sep, int *count);
tokenize:
    push %rbp
    mov %rsp, %rbp
    sub $64, %rsp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rdi, -8(%rbp)       
    movzbl %sil, %r12d       
    mov %rdx, -16(%rbp)      
    movl $0, -24(%rbp)       
    mov -8(%rbp), %rdi       
    call .skip_separators
    mov %rdi, -32(%rbp)
    cmpb $0, (%rdi)
    je .count_done
    incl -24(%rbp)           
.count_next:
    mov -32(%rbp), %rdi      
    call .find_separator
    test %rax, %rax
    jz .count_done
    mov %rax, %rdi
    inc %rdi
    mov %rdi, -32(%rbp)      
    call .skip_separators
    mov %rdi, -32(%rbp)      
    cmpb $0, (%rdi)
    je .count_done
    incl -24(%rbp)           
    jmp .count_next
.count_done:
    mov -16(%rbp), %rdx
    mov -24(%rbp), %eax
    movl %eax, (%rdx)
    movslq -24(%rbp), %rdi
    shl $3, %rdi             
    call malloc
    mov %rax, -40(%rbp)
    test %rax, %rax
    jz .done
    movl $0, -48(%rbp)       
    mov -8(%rbp), %rdi       
    call .skip_separators
    mov %rdi, -32(%rbp)      
    cmpb $0, (%rdi)
    je .done
.extract_loop:
    mov -32(%rbp), %rdi
    mov %rdi, -56(%rbp)      
    call .find_separator
    mov %rax, %rsi           
    mov %rsi, -64(%rbp)      
    test %rsi, %rsi
    jnz .not_last_token
    mov -56(%rbp), %rdi      
    call strlen
    mov %rax, -72(%rbp)      
    jmp .alloc_token
.not_last_token:
    
    mov -64(%rbp), %rax      
    mov -56(%rbp), %rdx      
    sub %rdx, %rax           
    mov %rax, -72(%rbp)     

.alloc_token:
    movq -72(%rbp), %rdi     
    inc %rdi                 
    call malloc
    movslq -48(%rbp), %rcx   
    mov -40(%rbp), %rdx      
    mov %rax, (%rdx, %rcx, 8)
    test %rax, %rax
    jz .next_token
    mov %rax, %rdi           
    mov -56(%rbp), %rsi      
    mov -72(%rbp), %rcx      
    test %rcx, %rcx
    jz .copy_done
    xor %r8, %r8             
.copy_loop:
    cmp %r8, %rcx
    je .copy_done
    movzbl (%rsi, %r8), %eax
    movb %al, (%rdi, %r8)
    inc %r8
    jmp .copy_loop
.copy_done:
    movslq -48(%rbp), %rcx   
    mov -40(%rbp), %rdx      
    mov (%rdx, %rcx, 8), %rdi 
    movq -72(%rbp), %rax     
    movb $0, (%rdi, %rax)
.next_token:
    incl -48(%rbp)           
    movl -48(%rbp), %eax
    cmpl -24(%rbp), %eax     
    jge .done
    mov -64(%rbp), %rsi
    test %rsi, %rsi
    jz .done
    lea 1(%rsi), %rdi
    call .skip_separators
    mov %rdi, -32(%rbp)      
    cmpb $0, (%rdi)
    je .done
    jmp .extract_loop
.done:
    mov -40(%rbp), %rax      
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    mov %rbp, %rsp
    pop %rbp
    ret
.find_separator:
    mov %rdi, %rsi           
    mov %r12b, %cl           
.find_loop:
    movzbl (%rsi), %eax      
    test %al, %al
    jz .find_end             
    cmpb %cl, %al            
    je .find_found           
    inc %rsi
    jmp .find_loop
.find_end:
    xor %rax, %rax           
    ret
.find_found:
    mov %rsi, %rax           
    ret

.skip_separators:
    mov %r12b, %cl           
.skip_loop:
    movzbl (%rdi), %eax      
    test %al, %al            
    jz .skip_done            
    cmpb %cl, %al            
    jne .skip_done           
    inc %rdi                 
    jmp .skip_loop
.skip_done:
    ret

.section .note.GNU-stack,"",@progbits


