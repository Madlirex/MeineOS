#pragma once
#include <stdint.h>

/**
 * Physical memory manager - Manages vacancy of individual memory pages.
 */
namespace PMM
{
constexpr uint64_t PAGE_SIZE = 4096; /**< Size of a memory page in bytes, must be 4096.*/

/**
 * Initializes memory page bitmap and marks all kernel/system pages as used.
 */
void init();

/**
 * Allocates a single memory page of size <code>PMM::PAGE_SIZE</code> and returns its physical address.
 * @return Physical address of allocated page, 0 if out of memory
 */
uint64_t alloc_frame();

/**
 * Finds a corresponding page to a physical address and frees it.
 * @param phys_addr Physical address, whose page is to be freed
 */
void free_frame(uint64_t phys_addr);

/**
 * Gets total memory of device.
 * @return Total memory in bytes
 */
uint64_t get_total_memory();
/**
 * Gets total free (unused) memory.
 * @return Total free memory in bytes
 */
uint64_t get_free_memory();
/**
 * Gets total used memory.
 * @return Total used memory in bytes
 */
uint64_t get_used_memory();
}
