[BITS 64]
extern isr_exception_handler
global isr_stub_table

; Macro for exceptions that DO NOT push an error code
%macro ISR_NOERRCODE 1
isr_stub_%1:
    push 0                  ; Dummy error code
    push %1                 ; Interrupt vector number
    jmp isr_common_stub
%endmacro

; Macro for exceptions that DO push an error code automatically
%macro ISR_ERRCODE 1
isr_stub_%1:
    push %1                 ; Interrupt vector number
    jmp isr_common_stub
%endmacro

; Define all 32 standard x86_64 CPU exceptions
ISR_NOERRCODE 0     ; Divide by Zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non-Maskable Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE   8     ; Double Fault
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment Not Present
ISR_ERRCODE   12    ; Stack-Segment Fault
ISR_ERRCODE   13    ; General Protection Fault
ISR_ERRCODE   14    ; Page Fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 Floating-Point Exception
ISR_ERRCODE   17    ; Alignment Check
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating-Point Exception
ISR_NOERRCODE 20    ; Virtualization Exception
ISR_ERRCODE   21    ; Control Protection Exception
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28    ; Hypervisor Injection Exception
ISR_ERRCODE   29    ; VMM Communication Exception
ISR_ERRCODE   30    ; Security Exception
ISR_NOERRCODE 31

isr_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp            ; Pass pointer to InterruptFrame struct as 1st C++ argument
    call isr_exception_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

; Array of 32 function pointers so C++ can loop over them easily
section .text
isr_stub_table:
%assign i 0
%rep 32
    dq isr_stub_%+i
%assign i i+1
%endrep
