[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [BOOT_DRIVE], dl

    ; Load Stage 2 (4 sectors = 2048 bytes) from LBA 1 (Sector 2) to 0x0000:0x8000
    mov si, dap_stage2
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Load Kernel (64 sectors = 32 KB) from LBA 5 (Sector 6) to 0x1000:0x0000 (phys 0x10000)
    mov si, dap_kernel
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000             ; Jump to Stage 2

disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    hlt

BOOT_DRIVE: db 0

; Disk Address Packet (DAP) for Stage 2
align 4
dap_stage2:
    db 0x10         ; Size of DAP (16 bytes)
    db 0            ; Always 0
    dw 4            ; Number of sectors to read (4 sectors = 2 KB)
    dw 0x8000       ; Offset
    dw 0x0000       ; Segment (0x0000:0x8000 = 0x8000)
    dq 1            ; Starting LBA (0 = boot sector, 1 = Sector 2)

; Disk Address Packet (DAP) for Kernel
align 4
dap_kernel:
    db 0x10         ; Size of DAP (16 bytes)
    db 0            ; Always 0
    dw 64           ; Number of sectors to read (64 sectors = 32 KB)
    dw 0x0000       ; Offset
    dw 0x1000       ; Segment (0x1000:0x0000 = 0x10000)
    dq 5            ; Starting LBA (5 = Sector 6, right after 1 boot + 4 stage2 sectors)

times 510 - ($ - $$) db 0
dw 0xAA55
