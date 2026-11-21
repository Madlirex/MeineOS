#include "boot_info.hpp"
#include "graphics.hpp"
#include "serial.hpp"
#include "idt.hpp"
#include <stdint.h>

using Color = Graphics::Color;

extern "C" __attribute__ ((section(".text.boot")))
void kernel_main()
{
    Serial::init();
    IDT::init();

    Serial::print("[KERNEL] Booted into 64-bit Long Mode!\n");
    Serial::print("[KERNEL] Framebuffer address: ");
    Serial::print_hex(BOOT_INFO->framebuffer);
    Serial::print("\n");

    for (uint32_t y = 0; y < BOOT_INFO->height; y++)
    {
        for (uint32_t x = 0; x < BOOT_INFO->width; x++)
        {
            Color col {
                static_cast<uint8_t>((x * 255) / BOOT_INFO->width),
                static_cast<uint8_t>((y * 255) / BOOT_INFO->height),
                180
            };
            Graphics::put_pixel(static_cast<int>(x), static_cast<int>(y), col);
        }
    }

    Graphics::fill_rect(210, 160, 600, 420, static_cast<Color>(0x0F181825)); // Shadow
    Graphics::fill_rect(200, 150, 600, 420, static_cast<Color>(0xECEFF4)); // Window body
    Graphics::fill_rect(200, 150, 600, 36, static_cast<Color>(0x2E3440)); // Dark title bar
    Graphics::fill_rect(772, 158, 20, 20, static_cast<Color>(0xBF616A)); // Red close button

    Graphics::fill_rect(0, static_cast<int>(BOOT_INFO->height - 40), static_cast<int>(BOOT_INFO->width), 40,
                        static_cast<Color>(0x242933));
    Graphics::fill_rect(8, static_cast<int>(BOOT_INFO->height - 34), 80, 28, static_cast<Color>(0x5E81AC));
    asm volatile("int $3");
    while (true)
    {
        asm volatile("hlt");
    }
}
