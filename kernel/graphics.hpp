#pragma once

#include <stdint.h>

/**
 * Provides trivial functions for drawing to screen.
 */
namespace Graphics
{
/**
 * An 8-bit RGBA color.
 */
struct Color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    /**
     * See-throughness from 0 (transparent) to 255 (solid).
     */
    uint8_t alpha {0xFF};

    constexpr Color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 0xFF) noexcept
        : red(r), green(g), blue(b), alpha(a) {}

    /**
     * Constructs a color from a packed 0xAARRGGBB hex value.
     * @param hex Packed 32-bit color value
     */
    explicit constexpr Color(const uint32_t hex) noexcept
        : red((hex >> 16) & 0xFF),
          green((hex >> 8) & 0xFF),
          blue(hex & 0xFF),
          alpha((hex >> 24) ? ((hex >> 24) & 0xFF) : 0xFF) {}

    /**
     * Packs the color into a 32-bit 0xAARRGGBB integer.
     * @return Packed color value
     */
    explicit constexpr operator uint32_t() const noexcept
    {
        return (static_cast<uint32_t>(alpha) << 24) |
            (static_cast<uint32_t>(red) << 16) |
            (static_cast<uint32_t>(green) << 8) |
            static_cast<uint32_t>(blue);
    }
};

/**
 * Fills a certain pixel on screen with a color.
 * @param x X coordinate of pixel
 * @param y Y coordinate of pixel
 * @param color Target color
 */
void put_pixel(int x, int y, Color color);
/**
 * Fills a rectangular area on screen with a color.
 * @param x Top-left X coordinate of rectangle
 * @param y Top-left Y coordinate of rectangle
 * @param w Width of rectangle
 * @param h Height of rectangle
 * @param color Target color
 */
void fill_rect(int x, int y, int w, int h, Color color);
}
