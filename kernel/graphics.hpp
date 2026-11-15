#pragma once

#include <stdint.h>

namespace Graphics
{
struct Color {
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

void put_pixel(int x, int y, Color col);
void fill_rect(int x, int y, int w, int h, Color color);
}