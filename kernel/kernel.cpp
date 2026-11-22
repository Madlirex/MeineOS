#include "boot_info.hpp"
#include "graphics.hpp"
#include "serial.hpp"
#include "idt.hpp"
#include "pmm.hpp"
#include <stdint.h>

using Color = Graphics::Color;

extern "C" __attribute__ ((section(".text.boot")))
void kernel_main()
{
    Serial::init();
    IDT::init();
    PMM::init();

    Serial::print("[KERNEL] Booted into 64-bit Long Mode!\n");
    Serial::print("[KERNEL] Framebuffer address: ");
    Serial::print_hex(BOOT_INFO->framebuffer);
    Serial::print("\n");

    while (true)
    {
        asm volatile("hlt");
    }
}
