[org 0x7c00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    mov ax, 0x0013
    int 0x10

    mov ah, 0x02
    mov al, 128
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13

    in al, 0x92
    or al, 2
    out 0x92, al

    lgdt [gdt32_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:init_32

[bits 32]
init_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7c00

    mov edi, 0x100000
    mov ecx, 3072
    xor eax, eax
    rep stosd

    mov dword [0x100000], 0x101003
    mov dword [0x101000], 0x102003
    mov dword [0x102000], 0x000083

    mov eax, 0x100000
    mov cr3, eax

    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lgdt [gdt64_desc]
    jmp 0x18:init_64

[bits 64]
init_64:
    mov rsp, 0x1F0000
    mov rbp, rsp

    mov rax, 0x10000
    call rax

halt_loop:
    hlt
    jmp halt_loop

boot_drive: db 0

gdt32:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt32_desc:
    dw $ - gdt32 - 1
    dd gdt32

gdt64:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x0020980000000000
gdt64_desc:
    dw $ - gdt64 - 1
    dd gdt64

times 510 - ($ - $$) db 0
dw 0xAA55