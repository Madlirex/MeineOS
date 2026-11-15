#pragma once
#include <stdint.h>

namespace Serial
{
void init();
bool is_transmit_empty();
void write_char(char c);
void print(const char* str);
void print_hex(uint64_t value);
}
