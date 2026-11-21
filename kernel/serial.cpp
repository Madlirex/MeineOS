#include "serial.hpp"
#include "io.hpp"
#include <stdint.h>

namespace Serial
{
constexpr uint16_t COM1 = 0x3F8;

void init()
{
    outb(COM1 + 1, 0x00); // Disable all interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00); // (hi byte)
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

bool is_transmit_empty()
{
    return (inb(COM1 + 5) & 0x20) != 0;
}

void write_char(char c)
{
    while (!is_transmit_empty())
    {
        asm volatile("pause");
    }
    outb(COM1, static_cast<uint8_t>(c));
}

void print(const char* str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        write_char(str[i]);
    }
}

void print_hex(const uint64_t value)
{
    auto hex_chars = "0123456789ABCDEF";
    print("0x");
    for (int i = 60; i >= 0; i -= 4)
    {
        write_char(hex_chars[(value >> i) & 0xF]);
    }
}
}
