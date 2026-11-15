[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Set up temporary 16-bit stack

    mov [BOOT_DRIVE], dl    ; Save boot drive number passed by BIOS

    ; Load Stage 2 + Kernel from disk (sectors 2..17) into 0x1000:0x0000 (phys 0x10000)
    mov ah, 0x02            ; BIOS read sectors function
    mov al, 16              ; Number of sectors to read (8 KB)
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Start at sector 2 (sector 1 is this boot sector)
    mov dh, 0               ; Head 0
    mov dl, [BOOT_DRIVE]
    mov bx, 0x1000
    mov es, bx
    xor bx, bx              ; ES:BX = 0x1000:0x0000 = 0x10000
    int 0x13
    jc disk_error

    ; Enable A20 line (Fast A20 gate)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Switch to 32-bit Protected Mode
    cli
    lgdt [gdt32_descriptor]
    mov eax, cr0
    or eax, 1               ; Set PE (Protection Enable) bit
    mov cr0, eax
    jmp 0x08:init_pm        ; Far jump to flush pipeline and load 32-bit CS

disk_error:
    mov ah, 0x0E
    mov al, 'E'             ; Print 'E' if disk read fails
    int 0x10
    hlt

BOOT_DRIVE: db 0

; 32-bit Global Descriptor Table
gdt32_start:
    dq 0x0                  ; Null descriptor
gdt32_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt32_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt32_end:

gdt32_descriptor:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

[BITS 32]
init_pm:
    mov ax, 0x10            ; 32-bit data segment selector
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000        ; Set up 32-bit stack

    ; Build 4-level page tables at 0x1000 (identity-mapping first 2 MB)
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096           ; Clear 16 KB (4 tables x 4096 bytes)
    rep stosd
    mov edi, cr3

    mov dword [edi], 0x2003         ; PML4[0] -> PDPT at 0x2000 (Present + RW)
    mov dword [0x2000], 0x3003      ; PDPT[0] -> PD at 0x3000 (Present + RW)
    mov dword [0x3000], 0x00000083  ; PD[0]   -> 2MB Huge Page at 0x0 (Present + RW + PS)

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set Long Mode bit in EFER MSR (Extended Feature Enable Register Model-Specific Register)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging (activates Long Mode)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load 64-bit GDT and jump to 64-bit C++ kernel at 0x10000
    lgdt [gdt64_descriptor]
    jmp 0x08:init_lm

gdt64_start:
    dq 0x0                          ; Null descriptor
    dq 0x00AF9A000000FFFF           ; 64-bit Code segment
    dq 0x00AF92000000FFFF           ; 64-bit Data segment
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

[BITS 64]
init_lm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x90000        ; Set up 64-bit stack
    jmp 0x10000             ; Jump directly to kernel_main at 0x10000

; Pad bootloader to exactly 512 bytes with boot signature
times 510 - ($ - $$) db 0
dw 0xAA55