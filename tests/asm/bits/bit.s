.section .data

.section .rodata

.section .bss

.section .text
    .global bitset, bitreset
# %rdi - ptr value
# %esi - bit
bitset:
    push %rbp
    mov %rsp, %rbp
    movq (%rdi), %rax
    movsxd %esi, %rsi
    btsq %rsi, %rax
    movq %rax, (%rdi)
    mov $0, %rax
    mov %rsp, %rbp
    pop %rbp
    ret
# rdi - ptr value
# esi - bit
bitreset:
    push %rbp
    mov %rsp, %rbp
    movq (%rdi), %rax
    movsxd %esi, %rsi
    btrq %rsi, %rax
    movq %rax, (%rdi)
    mov $0, %rax
    mov %rsp, %rbp
    pop %rbp
    ret

.section .note.GNU-stack,"",@progbits

