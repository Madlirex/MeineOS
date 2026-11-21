[BITS 16]
[ORG 0x8000]

; BootInfo at 0x7000 for kernel:
; [0x7000] uint64_t framebuffer_addr
; [0x7008] uint32_t width
; [0x700C] uint32_t height
; [0x7010] uint32_t pitch
; [0x7014] uint32_t bpp

BOOT_INFO equ 0x7000
VBE_MODE_INFO equ 0x7200      ; Temporary 256-byte buffer for VBE ModeInfoBlock
VBE_INFO equ 0x7400      ; 512-byte buffer for VBE Controller Info

; Cap the maximum resolution
MAX_WIDTH equ 1920
MAX_HEIGHT equ 1080

stage2_start:
    xor ax, ax
    mov es, ax

    ; Query VBE Controller Info (AX = 0x4F00)
    mov dword [VBE_INFO], 'VBE2'    ; Request VBE 2.0+ info
    mov ax, 0x4F00
    mov di, VBE_INFO
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Read far pointer (offset, segment) to the supported mode list at [VBE_INFO + 14]
    mov si, [VBE_INFO + 14]         ; Mode list offset
    mov fs, [VBE_INFO + 16]         ; Mode list segment

    xor bp, bp                      ; BP = best mode number found so far (0 = none)
    xor edx, edx                    ; EDX = best pixel count (width * height) so far

.find_mode_loop:
    mov cx, [fs:si]                 ; Read next 16-bit mode number from list
    add si, 2
    cmp cx, 0xFFFF                  ; 0xFFFF marks the end of the mode list
    je .done_searching

    ; Query info for mode CX (AX = 0x4F01)
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
    jne .find_mode_loop             ; Skip mode if query failed

    ; Check ModeAttributes (offset 0): must have bit 0 (supported) + bit 7 (linear FB)
    mov ax, [VBE_MODE_INFO + 0]
    and ax, 0x0081
    cmp ax, 0x0081
    jne .find_mode_loop

    ; Require 32 bits per pixel (offset 25)
    cmp byte [VBE_MODE_INFO + 25], 32
    jne .find_mode_loop

    ; Check width (offset 18) <= MAX_WIDTH and height (offset 20) <= MAX_HEIGHT
    movzx eax, word [VBE_MODE_INFO + 18]
    cmp eax, MAX_WIDTH
    ja .find_mode_loop

    movzx ebx, word [VBE_MODE_INFO + 20]
    cmp ebx, MAX_HEIGHT
    ja .find_mode_loop

    ; Calculate total pixels = width * height
    push edx
    mul ebx                         ; EAX = width * height (clobbers EDX)
    pop edx

    cmp eax, edx                    ; Is this mode larger than our best so far?
    jbe .find_mode_loop

    ; Save new best mode!
    mov edx, eax                    ; Update best pixel count
    mov bp, cx                      ; Update best mode number
    jmp .find_mode_loop

.done_searching:
    test bp, bp                     ; Check if atleast one is
    jz vbe_error

    ; Re-query the winning mode (BP) so VBE_MODE_INFO holds its exact data
    mov ax, 0x4F01
    mov cx, bp
    mov di, VBE_MODE_INFO
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Copy winning mode's framebuffer details into BOOT_INFO (0x7000)
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

    ; Switch to winning VBE mode with Linear Framebuffer bit (0x4000) enabled
    mov ax, 0x4F02
    mov bx, bp
    or bx, 0x4000
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

    ; Map 8 consecutive 2MB pages (16 MB total, safe for 1920x1080x32 and higher)
    mov ecx, 8
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

    ; Enable SSE so 64-bit C++ vector/float instructions never #UD
    mov rax, cr0
    and ax, 0xFFFB          ; Clear CR0.EM (bit 2)
    or ax, 0x2              ; Set CR0.MP (bit 1)
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9           ; Set CR4.OSFXSR (bit 9) and CR4.OSXMMEXCPT (bit 10)
    mov cr4, rax

    mov rax, 0x10000
    jmp rax                 ; Jump to kernel_main at 0x10000

; Pad Stage 2 to exactly 2048 bytes (4 sectors)
times 2048 - ($ - $$) db 0
