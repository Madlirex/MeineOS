[BITS 16]
[ORG 0x8000]

MMAP_BUFFER equ 0x6000          ; E820 entries stored here (up to 4 KB = 170 entries)
BOOT_INFO equ 0x7000            ; BootInfo struct for kernel
VBE_MODE_INFO equ 0x7200        ; 256-byte buffer for VBE ModeInfoBlock
VBE_INFO equ 0x7400             ; 512-byte buffer for VBE Controller Info

KERNEL_STAGE equ 0x10000        ; Where Stage 1 loaded the kernel below 1 MB
KERNEL_DEST equ 0x100000        ; Final 1 MB physical destination for kernel
KERNEL_MAX_DW equ 131072        ; Copy 512 KB (131,072 dwords) from 0x10000 -> 0x100000

MAX_WIDTH equ 1920
MAX_HEIGHT equ 1080

stage2_start:
    xor ax, ax
    mov es, ax

    ; Query BIOS E820 Physical Memory Map into 0x6000
    mov di, MMAP_BUFFER
    xor ebx, ebx                    ; Continuation value = 0
    xor bp, bp                      ; Entry counter = 0
    mov edx, 0x534D4150             ; 'SMAP' magic signature

.e820_loop:
    mov eax, 0xE820
    mov dword [es:di + 20], 1       ; Force ACPI 3.0 extended attribute = valid
    mov ecx, 24                     ; Request 24-byte entries
    int 0x15
    jc .e820_done                   ; Carry flag = end of list or unsupported
    mov edx, 0x534D4150             ; Restore 'SMAP' (some BIOSes clobber EDX)
    cmp eax, edx
    jne e820_error

    jcxz .e820_skip                 ; Ignore 0-length entries
    cmp cl, 20
    jbe .e820_good
    test byte [es:di + 20], 1       ; If ACPI 3.0 extended bit 0 is clear, ignore entry
    jz .e820_skip

.e820_good:
    inc bp
    add di, 24

.e820_skip:
    test ebx, ebx                   ; EBX == 0 means last entry reached
    jnz .e820_loop

.e820_done:
    test bp, bp
    jz e820_error

    ; Save memory map info into BOOT_INFO
    mov dword [BOOT_INFO + 24], MMAP_BUFFER
    mov dword [BOOT_INFO + 28], 0
    movzx eax, bp
    mov [BOOT_INFO + 32], eax

    ; Query VBE Controller Info & Find Best Mode
    mov dword [VBE_INFO], 'VBE2'
    mov ax, 0x4F00
    mov di, VBE_INFO
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    mov si, [VBE_INFO + 14]
    mov fs, [VBE_INFO + 16]

    xor bp, bp
    xor edx, edx

.find_mode_loop:
    mov cx, [fs:si]
    add si, 2
    cmp cx, 0xFFFF
    je .done_searching

    push cx
    push si
    push edx
    mov ax, 0x4F01
    mov di, VBE_MODE_INFO
    int 0x10
    pop edx
    pop si
    pop cx
    cmp ax, 0x004F
    jne .find_mode_loop

    mov ax, [VBE_MODE_INFO + 0]
    and ax, 0x0081
    cmp ax, 0x0081
    jne .find_mode_loop

    cmp byte [VBE_MODE_INFO + 25], 32
    jne .find_mode_loop

    movzx eax, word [VBE_MODE_INFO + 18]
    cmp eax, MAX_WIDTH
    ja .find_mode_loop

    movzx ebx, word [VBE_MODE_INFO + 20]
    cmp ebx, MAX_HEIGHT
    ja .find_mode_loop

    push edx
    mul ebx
    pop edx

    cmp eax, edx
    jbe .find_mode_loop

    mov edx, eax
    mov bp, cx
    jmp .find_mode_loop

.done_searching:
    test bp, bp
    jz vbe_error

    mov ax, 0x4F01
    mov cx, bp
    mov di, VBE_MODE_INFO
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Copy VBE framebuffer details into BOOT_INFO
    mov eax, [VBE_MODE_INFO + 40]
    mov [BOOT_INFO + 0], eax
    mov dword [BOOT_INFO + 4], 0

    movzx eax, word [VBE_MODE_INFO + 18]
    mov [BOOT_INFO + 8], eax

    movzx eax, word [VBE_MODE_INFO + 20]
    mov [BOOT_INFO + 12], eax

    movzx eax, word [VBE_MODE_INFO + 16]
    mov [BOOT_INFO + 16], eax

    movzx eax, byte [VBE_MODE_INFO + 25]
    mov [BOOT_INFO + 20], eax

    ; Switch to winning VBE mode
    mov ax, 0x4F02
    mov bx, bp
    or bx, 0x4000
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Enable A20 & Switch to 32-Bit Protected Mode
    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdt32_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_pm

e820_error:
    mov ah, 0x0E
    mov al, 'M'
    int 0x10
    hlt

vbe_error:
    mov ah, 0x0E
    mov al, 'V'
    int 0x10
    hlt

gdt32_start:
    dq 0x0
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
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000

    ; Copy Kernel from 0x10000 up to 0x100000 (1 MB)
    cld
    mov esi, KERNEL_STAGE
    mov edi, KERNEL_DEST
    mov ecx, KERNEL_MAX_DW
    rep movsd

    ; Build 64-Bit Page Tables (Map First 32 MB + Framebuffer)
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 5120
    rep stosd

    mov dword [0x1000], 0x2003      ; PML4[0] -> PDPT at 0x2000
    mov dword [0x2000], 0x3003      ; PDPT[0] -> PD_low at 0x3000

    ; Identity map first 32 MB (16 x 2 MB pages) so 0x100000+ is fully mapped
    mov edi, 0x3000
    mov eax, 0x00000083             ; Present + RW + 2MB Huge Page
    mov ecx, 16
.map_low_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_low_loop

    ; Map 16 MB at the VBE Framebuffer physical address
    mov ebx, [BOOT_INFO + 0]
    mov eax, ebx
    shr eax, 30
    mov dword [0x2000 + eax * 8], 0x4003

    mov eax, ebx
    shr eax, 21
    and eax, 0x1FF
    and ebx, 0xFFE00000
    or ebx, 0x83

    mov ecx, 8
.map_fb_loop:
    cmp eax, 512                    ; Guard against crossing 1 GB PD boundary
    jae .fb_map_done
    mov [0x4000 + eax * 8], ebx
    add ebx, 0x200000
    inc eax
    loop .map_fb_loop
.fb_map_done:

    ; Enable PAE, Long Mode, and Paging
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64_descriptor]
    jmp 0x08:init_lm

gdt64_start:
    dq 0x0
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF
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
    mov rsp, 0x90000

    ; Enable SSE
    mov rax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9
    mov cr4, rax

    ; Pass BootInfo* as 1st argument (RDI) and jump to 1 MB (0x100000)
    mov rdi, BOOT_INFO
    mov rax, KERNEL_DEST
    jmp rax

; Pad Stage 2 to exactly 2048 bytes (4 sectors)
times 2048 - ($ - $$) db 0
