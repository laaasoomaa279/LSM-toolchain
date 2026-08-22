.global _start
.extern main

.section .text.boot
.align 4
# Multiboot Header 32-bit
.long 0x1BADB002
.long 0x00000000
.long - (0x1BADB002 + 0x00000000)

_start:
    cli
    movl $stack_top, %esp
    movl %esp, %ebp

    # تصفير قسم .bss
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    call main

.halt_loop_32:
    cli
    hlt
    jmp .halt_loop_32

.section .bss
.align 16
stack_bottom:
    .skip 32768
stack_top: