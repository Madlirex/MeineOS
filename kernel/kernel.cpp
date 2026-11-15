#include <stdint.h>

volatile uint16_t* const VGA_BUFFER = reinterpret_cast<volatile uint16_t*>(0xB8000);
constexpr int VGA_WIDTH = 80;
constexpr int VGA_HEIGHT = 25;

void clear_screen(uint8_t color) {
    uint16_t blank = static_cast<uint16_t>(' ') | (static_cast<uint16_t>(color) << 8);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = blank;
    }
}

void print_string(const char* str, int row, int col, uint8_t color) {
    int index = row * VGA_WIDTH + col;
    for (int i = 0; str[i] != '\0'; i++) {
        VGA_BUFFER[index + i] = static_cast<uint16_t>(str[i]) | (static_cast<uint16_t>(color) << 8);
    }
}

extern "C" __attribute__((section(".text.boot"))) void kernel_main() {
    clear_screen(0x1F);
    print_string("Hello from 64-bit C++ Kernel!", 2, 4, 0x1F);
    print_string("Running in Long Mode from scratch.", 4, 4, 0x1E);

    while (true) {
        asm volatile("hlt");
    }
}
