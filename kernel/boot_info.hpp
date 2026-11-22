#pragma once
#include <stdint.h>

/**
 * Physical memory map entry reported by BIOS INT 0x15, EAX=0xE820.
 */
struct __attribute__((packed)) E820Entry
{
    uint64_t base;     /**< Physical start address of the memory region */
    uint64_t length;   /**< Size of the memory region in bytes */
    uint32_t type;     /**< Region type (1 = Usable RAM, 2 = Reserved, 3 = ACPI Reclaimable, 4 = ACPI NVS, 5 = Bad Memory) */
    uint32_t acpi_ext; /**< ACPI 3.0 extended attributes bitfield (bit 0 = ignore entry if 0) */
};

/**
 * Hardware, framebuffer, and memory map information passed by the bootloader.
 */
struct __attribute__((packed)) BootInfo
{
    uint64_t framebuffer;  /**< Physical base address of the linear framebuffer */
    uint32_t width;        /**< Visible screen width in pixels */
    uint32_t height;       /**< Visible screen height in pixels */
    uint32_t pitch;        /**< Bytes per horizontal scanline, including any padding */
    uint32_t bpp;          /**< Color depth in bits per pixel */
    uint64_t mmap_addr;    /**< Physical address of the E820Entry array */
    uint32_t mmap_entries; /**< Number of valid entries in the E820 memory map */
};

/** Read-only pointer to the boot information structure placed at 0x7000 by the bootloader. */
inline const auto* BOOT_INFO = reinterpret_cast<const BootInfo*>(0x7000);
