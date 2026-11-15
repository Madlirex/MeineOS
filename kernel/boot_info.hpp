#pragma once
#include <stdint.h>

struct __attribute__((packed)) BootInfo {
    uint64_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

inline auto* const BOOT_INFO = reinterpret_cast<BootInfo*>(0x7000);
