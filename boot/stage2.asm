[BITS 16]
[ORG 0x8000]

; BootInfo at 0x7000 for kernel:
; [0x7000] uint64_t framebuffer_addr
; [0x7008] uint32_t width
; [0x700C] uint32_t height
; [0x7010] uint32_t pitch
; [0x7014] uint32_t bpp

BOOT_INFO equ 0x7000
VBE_MODE_INFO equ 0x7200      ; Temporary 256-byte buffer for BIOS VBE query

stage2_start:
    ; Query VBE Mode 0x118 (1024x768, 24/32-bit RGB)
    mov ax, 0x4F01
    mov cx, 0x118
    mov di, VBE_MODE_INFO
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Extract framebuffer details from VBE ModeInfoBlock into BOOT_INFO (0x7000)
    mov eax, [VBE_MODE_INFO + 40]   ; Physical base pointer (32-bit)
    mov [BOOT_INFO + 0], eax
    mov dword [BOOT_INFO + 4], 0    ; Upper 32 bits of 64-bit pointer = 0

    movzx eax, word [VBE_MODE_INFO + 18] ; XResolution (width)
    mov [BOOT_INFO + 8], eax

    movzx eax, word [VBE_MODE_INFO + 20] ; YResolution (height)
    mov [BOOT_INFO + 12], eax

    movzx eax, word [VBE_MODE_INFO + 16] ; BytesPerScanLine (pitch)
    mov [BOOT_INFO + 16], eax

    movzx eax, byte [VBE_MODE_INFO + 25] ; BitsPerPixel (bpp)
    mov [BOOT_INFO + 20], eax

    ; Switch to VBE Mode 0x118 with Linear Framebuffer bit (0x4000) enabled
    mov ax, 0x4F02
    mov bx, 0x118 | 0x4000
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Switch to 32-bit Protected Mode
    cli
    lgdt [gdt32_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_pm

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

    ; Clear 20 KB for Page Tables at 0x1000..0x5FFF
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 5120
    rep stosd
    mov edi, cr3

    ; Identity map first 2 MB (0x00000000 - 0x001FFFFF)
    mov dword [0x1000], 0x2003      ; PML4[0] -> PDPT at 0x2000
    mov dword [0x2000], 0x3003      ; PDPT[0] -> PD_low at 0x3000
    mov dword [0x3000], 0x00000083  ; PD_low[0] -> 2MB page at 0x0

    ; Dynamically map 8 MB at the VBE Framebuffer physical address (e.g., 0xFD000000)
    mov ebx, [BOOT_INFO + 0]        ; EBX = Framebuffer physical address
    mov eax, ebx
    shr eax, 30                     ; Bits 30..38 = PDPT index (e.g., 3 for 0xFD000000)
    mov dword [0x2000 + eax * 8], 0x4003 ; PDPT[idx] -> PD_fb at 0x4000

    mov eax, ebx
    shr eax, 21
    and eax, 0x1FF                  ; Bits 21..29 = PD index
    and ebx, 0xFFE00000             ; Align framebuffer base to 2 MB boundary
    or ebx, 0x83                    ; Present + RW + 2MB Huge Page

    ; Map 4 consecutive 2MB pages (8 MB total, plenty for 1024x768x32 = 3 MB)
    mov ecx, 4
.map_fb_loop:
    mov [0x4000 + eax * 8], ebx
    add ebx, 0x200000
    inc eax
    loop .map_fb_loop

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable Long Mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging
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
    jmp 0x8800                      ; Jump to kernel_main at 0x8800!

; Pad Stage 2 to exactly 2048 bytes (4 sectors)
times 2048 - ($ - $$) db 0
