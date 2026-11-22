#pragma once

#include <stdint.h>

/**
 * Writes a byte to the specified hardware I/O port.
 * @param port Target I/O port address
 * @param value Byte to send
 */
inline void outb(uint16_t port, uint8_t value) noexcept { asm volatile("outb %0, %1" : : "a"(value), "Nd"(port)); }

/**
 * Reads a byte from the specified hardware I/O port.
 * @param port Source I/O port address
 * @return Byte read from the port
 */
inline uint8_t inb(uint16_t port) noexcept
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Waits for a brief I/O cycle (~1-4 microseconds) to let slow legacy hardware settle.
 * Writes a dummy byte to unused diagnostic port 0x80.
 */
inline void io_wait() noexcept { outb(0x80, 0); }
