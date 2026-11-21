#pragma once

#include <stdint.h>

namespace IDT
{
struct __attribute__((packed)) Entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct __attribute__((packed)) Pointer
{
    uint16_t limit;
    uint64_t base;
};

struct __attribute__((packed)) InterruptFrame
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void init();
void set_gate(uint8_t vector, void* handler, uint8_t flags = 0x8E);
}
