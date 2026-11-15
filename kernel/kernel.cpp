#include <stdint.h>

struct __attribute__((packed)) BootInfo {
    uint64_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

struct Color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha { 0xFF };

    constexpr Color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 0xFF) noexcept
        : red(r), green(g), blue(b), alpha(a) {}

    explicit constexpr Color(const uint32_t hex) noexcept
        : red((hex >> 16) & 0xFF),
          green((hex >> 8) & 0xFF),
          blue(hex & 0xFF),
          alpha((hex >> 24) ? ((hex >> 24) & 0xFF) : 0xFF) {}

    explicit constexpr operator uint32_t() const noexcept {
        return (static_cast<uint32_t>(alpha) << 24) |
               (static_cast<uint32_t>(red)   << 16) |
               (static_cast<uint32_t>(green) << 8)  |
                static_cast<uint32_t>(blue);
    }
};

auto* const BOOT_INFO = reinterpret_cast<BootInfo*>(0x7000);

void put_pixel(const int x, const int y, const Color color) {

    if (x < 0 || y < 0 || static_cast<uint32_t>(x) >= BOOT_INFO->width || static_cast<uint32_t>(y) >= BOOT_INFO->height) {
        return;
    }

    auto* fb = reinterpret_cast<uint8_t*>(BOOT_INFO->framebuffer);
    const uint32_t bytes_per_pixel = BOOT_INFO->bpp / 8;
    uint8_t* pixel = fb + (y * BOOT_INFO->pitch) + (x * bytes_per_pixel);

    // BGRA layout
    if (color.alpha == 255) {
        pixel[0] = color.blue;
        pixel[1] = color.green;
        pixel[2] = color.red;
        if (bytes_per_pixel == 4) {
            pixel[3] = 0xFF;
        }
        return;
    }

    const uint32_t inv_alpha = 255 - color.alpha;
    pixel[0] = static_cast<uint8_t>((color.blue  * color.alpha + pixel[0] * inv_alpha) / 255);
    pixel[1] = static_cast<uint8_t>((color.green * color.alpha + pixel[1] * inv_alpha) / 255);
    pixel[2] = static_cast<uint8_t>((color.red * color.alpha + pixel[2] * inv_alpha) / 255);
}

void fill_rect(const int x, const int y, const int w, const int h, const Color color) {
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            put_pixel(col, row, color);
        }
    }
}

extern "C" __attribute__((section(".text.boot"))) void kernel_main() {
    for (uint32_t y = 0; y < BOOT_INFO->height; y++) {
        for (uint32_t x = 0; x < BOOT_INFO->width; x++) {
            Color col {
                static_cast<uint8_t>((x * 255) / BOOT_INFO->width),
                static_cast<uint8_t>((y * 255) / BOOT_INFO->height),
                180
            };
            put_pixel(static_cast<int>(x), static_cast<int>(y), col);
        }
    }

    fill_rect(210, 160, 600, 420, static_cast<Color>(0x0F181825)); // Shadow
    fill_rect(200, 150, 600, 420, static_cast<Color>(0xECEFF4)); // Window body
    fill_rect(200, 150, 600, 36, static_cast<Color>(0x2E3440)); // Dark title bar
    fill_rect(772, 158, 20, 20, static_cast<Color>(0xBF616A)); // Red close button

    fill_rect(0, static_cast<int>(BOOT_INFO->height - 40), static_cast<int>(BOOT_INFO->width), 40, static_cast<Color>(0x242933));
    fill_rect(8, static_cast<int>(BOOT_INFO->height - 34), 80, 28, static_cast<Color>(0x5E81AC));

    while (true) {
        asm volatile("hlt");
    }
}
