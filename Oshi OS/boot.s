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

    mov dx, 0x01CE
    mov ax, 4
    out dx, ax
    mov dx, 0x01CF
    mov ax, 0
    out dx, ax

    mov dx, 0x01CE
    mov ax, 1
    out dx, ax
    mov dx, 0x01CF
    mov ax, 1920
    out dx, ax

    mov dx, 0x01CE
    mov ax, 2
    out dx, ax
    mov dx, 0x01CF
    mov ax, 1080
    out dx, ax

    mov dx, 0x01CE
    mov ax, 3
    out dx, ax
    mov dx, 0x01CF
    mov ax, 32
    out dx, ax

    mov dx, 0x01CE
    mov ax, 4
    out dx, ax
    mov dx, 0x01CF
    mov ax, 0x41
    out dx, ax

    push ds
    mov ax, 0x1130
    mov bh, 0x06
    int 0x10
    mov ax, es
    mov ds, ax
    mov si, bp
    xor ax, ax
    mov es, ax
    mov di, 0x8000
    mov cx, 1024
    rep movsd
    pop ds

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

    mov edi, 0x9000
    mov ecx, 5120
    xor eax, eax
    rep stosd

    mov dword [0x9000], 0x0000A003
    mov dword [0xA000], 0x0000B003
    mov dword [0xA008], 0x0000C003
    mov dword [0xA010], 0x0000D003
    mov dword [0xA018], 0x0000E003

    mov edi, 0xB000
    mov eax, 0x00000083
    mov ecx, 2048

.map_4gb:
    mov dword [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .map_4gb

    mov eax, 0x9000
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
    mov rsp, 0x90000
    mov rbp, rsp
    and rsp, -16

    mov rax, 0x10000
    call rax

halt_loop:
    cli
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