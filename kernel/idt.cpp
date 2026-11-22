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
void set_gate(const uint8_t vector, void* handler, const uint8_t flags) noexcept
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

void init() noexcept
{
    auto* raw = reinterpret_cast<uint8_t*>(&idt);
    for (uint64_t i = 0; i < sizeof(idt); i++) { raw[i] = 0; }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = reinterpret_cast<uint64_t>(&idt);

    for (uint8_t i = 0; i < 32; i++) { set_gate(i, isr_stub_table[i]); }

    asm volatile("lidt %0" : : "m"(idt_ptr));
    Serial::print("[IDT] 64-bit Interrupt Descriptor Table loaded.\n");
}
}

extern "C" void isr_exception_handler(IDT::InterruptFrame* frame)
{
    Serial::print("\n================ KERNEL PANIC ================\n");
    Serial::print("Exception: ");
    Serial::print(frame->vector < 32 ? EXCEPTION_NAMES[frame->vector] : "Unknown");
    Serial::print("\nVector: ");
    Serial::print_hex(frame->vector);
    Serial::print(" | Error Code: ");
    Serial::print_hex(frame->error_code);
    Serial::print("\nRIP: ");
    Serial::print_hex(frame->rip);
    Serial::print(" | RSP: ");
    Serial::print_hex(frame->rsp);
    Serial::print("\n==============================================\n");

    auto white = Graphics::Color(0xFFFFFFFF);

    Graphics::fill_rect(0, 0, static_cast<int>(BOOT_INFO->width), static_cast<int>(BOOT_INFO->height),
                        Graphics::Color(0x0050CC));
    Graphics::draw_char(100, 100, ':', white, 40);
    Graphics::draw_char(260, 100, '(', white, 40);
    Graphics::draw_text(620, 100, "Oops!", white, 10);
    Graphics::draw_text(620, 196, "It seems like the system ran into a problem", white, 3);
    Graphics::draw_text(620, 356, "Exception:", white, 3);
    Graphics::draw_text(916, 356, frame->vector < 32 ? EXCEPTION_NAMES[frame->vector] : "Unknown", white, 3);

    Graphics::draw_text(620, 396, "Vector:", white, 3);
    Graphics::draw_hex(916, 396, frame->vector, white, 3);

    Graphics::draw_text(620, 436, "Error Code:", white, 3);
    Graphics::draw_hex(916, 436, frame->error_code, white, 3);

    Graphics::draw_text(620, 476, "RIP:", white, 3);
    Graphics::draw_hex(916, 476, frame->rip, white, 3);

    Graphics::draw_text(620, 516, "RSP:", white, 3);
    Graphics::draw_hex(916, 516, frame->rsp, white, 3);

    while (true) { asm volatile("cli; hlt"); }
}
