#pragma once
#include <stdint.h>

/**
 * Hardware and framebuffer information passed by the bootloader.
 */
struct __attribute__((packed)) BootInfo {
    uint64_t framebuffer; /**< Physical base address of the linear framebuffer */
    uint32_t width;       /**< Visible screen width in pixels */
    uint32_t height;      /**< Visible screen height in pixels */
    uint32_t pitch;       /**< Bytes per horizontal scanline, including any padding */
    uint32_t bpp;         /**< Color depth in bits per pixel */
};

/** Pointer to the boot information structure placed in memory by the bootloader. */
inline auto* const BOOT_INFO = reinterpret_cast<BootInfo*>(0x7000);
