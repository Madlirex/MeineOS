#include "graphics.hpp"
#include "boot_info.hpp"

namespace Graphics
{
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

    if (bytes_per_pixel == 4)
    {
        pixel[3] = color.alpha;
    }
}

void fill_rect(const int x, const int y, const int w, const int h, const Color color) {
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            put_pixel(col, row, color);
        }
    }
}
}
