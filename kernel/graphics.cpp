#include "graphics.hpp"
#include "boot_info.hpp"
#include "font.hpp"

namespace Graphics
{
void put_pixel(const int x, const int y, const Color color)
{
    if (x < 0 || y < 0 || static_cast<uint32_t>(x) >= BOOT_INFO->width || static_cast<uint32_t>(y) >= BOOT_INFO->height)
    {
        return;
    }

    auto* fb = reinterpret_cast<uint8_t*>(BOOT_INFO->framebuffer);
    const uint32_t bytes_per_pixel = BOOT_INFO->bpp / 8;
    uint8_t* pixel = fb + (y * BOOT_INFO->pitch) + (x * bytes_per_pixel);

    // BGRA layout
    if (color.alpha == 255)
    {
        pixel[0] = color.blue;
        pixel[1] = color.green;
        pixel[2] = color.red;
        if (bytes_per_pixel == 4) { pixel[3] = 0xFF; }
        return;
    }

    const uint32_t inv_alpha = 255 - color.alpha;
    pixel[0] = static_cast<uint8_t>((color.blue * color.alpha + pixel[0] * inv_alpha) / 255);
    pixel[1] = static_cast<uint8_t>((color.green * color.alpha + pixel[1] * inv_alpha) / 255);
    pixel[2] = static_cast<uint8_t>((color.red * color.alpha + pixel[2] * inv_alpha) / 255);

    if (bytes_per_pixel == 4) { pixel[3] = color.alpha; }
}

void fill_rect(const int x, const int y, const int w, const int h, const Color color)
{
    for (int row = y; row < y + h; row++) { for (int col = x; col < x + w; col++) { put_pixel(col, row, color); } }
}

void draw_char(const int x, const int y, char c, const Color color, const int scale)
{
    if (c < 32 || c > 126)
        c = '?';
    const uint8_t* glyph = FONT_8X8[c - 32];
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (glyph[row] & (1 << col))
            {
                if (scale == 1)
                    put_pixel(x + col, y + row, color);
                else
                    fill_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void draw_text(const int x, const int y, const char* text, const Color color, const int scale)
{
    if (!text) return;

    int cursor_x = x;
    int cursor_y = y;
    const int char_step = 8 * scale;

    for (int i = 0; text[i] != '\0'; i++)
    {
        const char c = text[i];

        if (c == '\n')
        {
            cursor_x = x;
            cursor_y += char_step;
        }
        else if (c == '\r') { cursor_x = x; }
        else if (c == '\t')
        {
            const int tab_width = 4 * char_step;
            cursor_x = x + ((cursor_x - x) / tab_width + 1) * tab_width;
        }
        else if (c == ' ') { cursor_x += char_step; }
        else
        {
            draw_char(cursor_x, cursor_y, c, color, scale);
            cursor_x += char_step;
        }
    }
}

void draw_hex(const int x, const int y, const uint64_t value, const Color color, const int scale)
{
    const char hex_digits[] = "0123456789ABCDEF";
    char buffer[19];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[18] = '\0';

    for (int i = 0; i < 16; i++)
    {
        const uint8_t digit = value >> ((15 - i) * 4) & 0xF;
        buffer[i + 2] = hex_digits[digit];
    }

    draw_text(x, y, buffer, color, scale);
}
}
