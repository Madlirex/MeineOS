#pragma once
#include "pmm.hpp"
#include <stdint.h>

/**
 * Virtual Memory Manager (VMM) - Manages 4-level x86-64 paging, virtual-to-physical
 * address translation, and higher-half direct memory mapping.
 */
namespace VMM
{
constexpr uint64_t PAGE_SIZE = PMM::PAGE_SIZE; /**< Size of a memory page in bytes, must be 4096. @see PMM::PAGE_SIZE */
constexpr uint64_t HHDM_OFFSET = 0xFFFF800000000000ULL; /**< Base offset for the Higher-Half Direct Map (HHDM) */

/**
 * Hardware bitmask flags for x86-64 page table entries (PML4, PDPT, PD, and PT).
 */
enum PageFlags : uint64_t
{
    PRESENT = 1ULL << 0, /**< Entry is valid and mapped in physical memory */
    WRITABLE = 1ULL << 1, /**< Grants write access, otherwise read-only */
    USER = 1ULL << 2, /**< Grants Ring 3 user-mode access, otherwise Ring 0 only */
    WRITE_THROUGH = 1ULL << 3, /**< Enables write-through caching instead of write-back */
    CACHE_DISABLE = 1ULL << 4, /**< Disables CPU caching for this page (required for MMIO regions) */
    ACCESSED = 1ULL << 5, /**< Set by CPU hardware whenever the page is read or written */
    DIRTY = 1ULL << 6, /**< Set by CPU hardware whenever the page is written to (leaf entries only) */
    HUGE_PAGE = 1ULL << 7, /**< 2MB page in PD, or 1GB page in PDPT */
    GLOBAL = 1ULL << 8, /**< Prevents TLB flush when CR3 is reloaded (requires CR4.PGE) */
    NO_EXECUTE = 1ULL << 63 /**< Blocks instruction execution from this page (requires EFER.NXE) */
};

/**
 * Converts a physical address to its HHDM virtual pointer.
 * @param phys Physical address to convert
 * @return Virtual pointer inside the Higher-Half Direct Map
 */
inline void* phys_to_virt(const uint64_t phys) { return reinterpret_cast<void*>(phys + HHDM_OFFSET); }

/**
 * Converts an HHDM virtual pointer back to its physical address.
 * @note Only valid for addresses within the Higher-Half Direct Map. @see virt_to_phys
 * @param virt Virtual pointer inside the HHDM
 * @return Corresponding physical address
 */
inline uint64_t virt_to_phys_hhdm(const void* virt) { return reinterpret_cast<uint64_t>(virt) - HHDM_OFFSET; }

/**
 * Returns the physical address of the kernel's root PML4 table.
 * @return Physical address of kernel PML4
 */
uint64_t get_kernel_pml4();

/**
 * Initializes the kernel PML4, identity-maps low kernel memory, maps physical RAM
 * into the HHDM, maps the framebuffer, and activates the new page tables in CR3.
 */
void init();

/**
 * Maps a 4KB physical frame in the kernel's default PML4 table.
 * @param virt_addr Target virtual address (page-aligned)
 * @param phys_addr Target physical frame address (page-aligned)
 * @param flags Page entry bitmask flags @see PageFlags
 * @see unmap_page
 */
void map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
/**
 * Maps a 4KB physical frame to a virtual address in the specified page table hierarchy.
 * @param virt_addr Target virtual address (page-aligned)
 * @param phys_addr Target physical frame address (page-aligned)
 * @param flags Page entry bitmask flags @see PageFlags
 * @param pml4_phys Physical address of target PML4 table
 * @see unmap_page
 */
void map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags, uint64_t pml4_phys);

/**
 * Unmaps a 4KB virtual page from the kernel's default PML4 table.
 * @param virt_addr Virtual address to unmap (page-aligned)
 * @see map_page
 */
void unmap_page(uint64_t virt_addr);
/**
 * Unmaps a 4KB virtual page from the specified page table hierarchy.
 * @param virt_addr Virtual address to unmap (page-aligned)
 * @param pml4_phys Physical address of target PML4 table
 * @see map_page
 */
void unmap_page(uint64_t virt_addr, uint64_t pml4_phys);

/**
 * Walks the kernel's page tables to translate any virtual address into its physical address.
 * @param virt_addr Virtual address to translate
 * @return Physical address, or 0 if the virtual address is not mapped
 */
uint64_t virt_to_phys(uint64_t virt_addr);
/**
 * Walks the page tables to translate any virtual address into its physical address.
 * @param virt_addr Virtual address to translate
 * @param pml4_phys Physical address of target PML4 table
 * @return Physical address, or 0 if the virtual address is not mapped
 */
uint64_t virt_to_phys(uint64_t virt_addr, uint64_t pml4_phys);
}
