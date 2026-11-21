#pragma once

#include <stdint.h>

namespace IDT
{
/**
 * 64-bit Interrupt Descriptor Table (IDT) gate entry.
 */
struct __attribute__((packed)) Entry
{
    uint16_t offset_low;  /**< Bits 0..15 of the handler virtual address */
    uint16_t selector;    /**< Kernel code segment selector in the GDT */
    uint8_t ist;          /**< Interrupt Stack Table offset (0 = legacy stack switching) */
    uint8_t type_attr;    /**< Gate type, DPL privilege level, and present bit */
    uint16_t offset_mid;  /**< Bits 16..31 of the handler virtual address */
    uint32_t offset_high; /**< Bits 32..63 of the handler virtual address */
    uint32_t zero;        /**< Reserved by hardware; must be 0 */
};

/**
 * Descriptor structure loaded into the CPU's IDTR register via LIDT.
 */
struct __attribute__((packed)) Pointer
{
    uint16_t limit; /**< Size of the IDT in bytes minus 1 */
    uint64_t base;  /**< Linear base address of the IDT entry array */
};

/**
 * CPU register state pushed onto the stack when an interrupt or exception fires.
 */
struct __attribute__((packed)) InterruptFrame
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8; /**< General-purpose registers saved by ISR stub */
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;    /**< General-purpose registers saved by ISR stub */
    uint64_t vector;                               /**< Interrupt vector number (0..255) */
    uint64_t error_code;                           /**< Hardware error code (or 0 if pushed by stub) */
    uint64_t rip, cs, rflags, rsp, ss;             /**< CPU-pushed iretq return frame */
};

/**
 * Initializes the default exception/IRQ handlers and loads the IDT into the CPU.
 */
void init() noexcept;

/**
 * Registers an interrupt handler function at the specified IDT vector.
 * @param vector Interrupt vector index (0..255)
 * @param handler Pointer to the ISR entry stub
 * @param flags Gate type and privilege attributes (defaults to 0x8E: 64-bit Ring 0 interrupt gate)
 */
void set_gate(uint8_t vector, void* handler, uint8_t flags = 0x8E) noexcept;
} // namespace IDT
