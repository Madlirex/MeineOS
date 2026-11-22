#include "boot_info.hpp"
#include "serial.hpp"
#include "idt.hpp"
#include "pmm.hpp"
#include "vmm.hpp"

extern "C" __attribute__ ((section
(
".text.boot"
)
)
)
void kernel_main()
{
    Serial::init();
    IDT::init();
    PMM::init();
    VMM::init();

    Serial::print("[KERNEL] Booted into 64-bit Long Mode!\n");
    Serial::print("[KERNEL] Framebuffer address: ");
    Serial::print_hex(BOOT_INFO->framebuffer);
    Serial::print("\n");

    asm volatile("int $8");
    while (true) { asm volatile("hlt"); }
}
