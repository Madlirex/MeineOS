[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [BOOT_DRIVE], dl

    ; Read 32 sectors (16 KB) starting at Sector 2 into 0x0000:0x8000
    ; Sectors 2..5  = Stage 2 (2048 bytes, at 0x8000)
    ; Sectors 6..33 = Kernel (at 0x8800)
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    mov bx, 0x8000
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000         ; Jump to Stage 2

disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    hlt

BOOT_DRIVE: db 0

times 510 - ($ - $$) db 0
dw 0xAA55
