#pragma once
#include <stdint.h>

namespace Serial
{
/**
 * Initializes the COM1 serial port at 38400 baud, 8N1 mode.
 */
void init() noexcept;

/**
 * Checks whether the serial transmit buffer is ready for a new byte.
 * @return True if the transmit buffer is empty, false otherwise
 */
bool is_transmit_empty() noexcept;

/**
 * Blocks until the transmit buffer is ready, then sends a single character.
 * @param c Character to transmit
 */
void write_char(char c) noexcept;

/**
 * Writes a null-terminated string to the serial port.
 * @param str Pointer to the null-terminated string
 */
void print(const char* str) noexcept;

/**
 * Formats and writes a 64-bit integer as a hexadecimal string (e.g., 0x0000000000000000).
 * @param value Integer value to print in base 16
 */
void print_hex(uint64_t value) noexcept;
} // namespace Serial
