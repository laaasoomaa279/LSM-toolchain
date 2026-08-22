.global _start
.extern main
.extern __bss_start
.extern __bss_end

.section .text.boot
_start:
    cli

    leaq _stack_top(%rip), %rsp
    movq %rsp, %rbp

    leaq __bss_start(%rip), %rdi
    leaq __bss_end(%rip), %rcx
    subq %rdi, %rcx
    shrq $3, %rcx
    xorq %rax, %rax
    rep stosq

    andq $-16, %rsp
    subq $8, %rsp

    call main

.halt_loop:
    cli
    hlt
    jmp .halt_loop

.section .bss
.align 16
_stack_bottom:
    .skip 65536
_stack_top: