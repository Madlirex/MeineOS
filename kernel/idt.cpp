#include "idt.hpp"
#include "serial.hpp"
#include "graphics.hpp"
#include "boot_info.hpp"

extern "C" void* isr_stub_table[32];

namespace
{
IDT::Entry idt[256] {};
IDT::Pointer idt_ptr {};

const char* EXCEPTION_NAMES[32] = {
    "Divide by Zero", "Debug", "Non-Maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
    "x87 Floating-Point", "Alignment Check", "Machine Check", "SIMD Floating-Point",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection", "VMM Communication", "Security Exception", "Reserved"
};
}

namespace IDT
{
void set_gate(const uint8_t vector, void* handler, const uint8_t flags)
{
    const auto addr = reinterpret_cast<uint64_t>(handler);
    idt[vector].offset_low = addr & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].ist = 0;
    idt[vector].type_attr = flags;
    idt[vector].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

void init()
{
    auto* raw = reinterpret_cast<uint8_t*>(&idt);
    for (uint64_t i = 0; i < sizeof(idt); i++)
    {
        raw[i] = 0;
    }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = reinterpret_cast<uint64_t>(&idt);

    for (uint8_t i = 0; i < 32; i++)
    {
        set_gate(i, isr_stub_table[i]);
    }

    asm volatile("lidt %0" : : "m"(idt_ptr));
    Serial::print("[IDT] 64-bit Interrupt Descriptor Table loaded.\n");
}
}

extern "C" void isr_exception_handler(IDT::InterruptFrame* frame)
{
    Serial::print("\n================ KERNEL PANIC ================\n");
    Serial::print("Exception: ");
    if (frame->vector < 32)
    {
        Serial::print(EXCEPTION_NAMES[frame->vector]);
    }
    Serial::print("\nVector: ");
    Serial::print_hex(frame->vector);
    Serial::print(" | Error Code: ");
    Serial::print_hex(frame->error_code);
    Serial::print("\nRIP: ");
    Serial::print_hex(frame->rip);
    Serial::print(" | RSP: ");
    Serial::print_hex(frame->rsp);
    Serial::print("\n==============================================\n");

    Graphics::fill_rect(0, 0, static_cast<int>(BOOT_INFO->width), static_cast<int>(BOOT_INFO->height),
                        Graphics::Color(0x0050CC));
    Graphics::fill_rect(100, 100, 120, 120, Graphics::Color(0xFFFFFF)); // Classic white square/sad face badge

    while (true)
    {
        asm volatile("cli; hlt");
    }
}
