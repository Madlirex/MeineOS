#include "pmm.hpp"
#include "boot_info.hpp"
#include "serial.hpp"

extern "C" uint8_t kernel_start[]; /**< Address of start of kernel */
extern "C" uint8_t kernel_end[]; /**< Address of end of kernel */

namespace
{
uint8_t* bitmap = nullptr; /**< Pointer to array of bits, each representing memory page's state (0 = free, 1 = used) */
uint64_t total_pages = 0; /**< Total amount of memory pages */
uint64_t used_pages = 0; /**< Amount of used memory pages */
uint64_t last_free_page = 0; /**< Cached index of last free memory page for faster scanning */

/**
 * Marks a certain memory page as used.
 * @param page Index of page to be marked
 * @see bitmap_clear, bitmap_test
 */
inline void bitmap_set(const uint64_t page) { bitmap[page / 8] |= (1ULL << (page % 8)); }

/**
 * Marks a certain memory page as free.
 * @param page Index of page to be marked
 * @see bitmap_set, bitmap_test
 */
inline void bitmap_clear(const uint64_t page) { bitmap[page / 8] &= ~(1ULL << (page % 8)); }

/**
 * Tests a certain memory page to see whether it's free or used.
 * @param page Index of page to be tested
 * @return <code>true</code> if page is used, <code>false</code> if free
 * @see bitmap_set, bitmap_clear
 */
inline bool bitmap_test(const uint64_t page) { return (bitmap[page / 8] & (1ULL << (page % 8))) != 0; }

/**
 * Marks a memory region as free.
 * Aligned narrowly, so that if <code>base</code> or <code>base + length</code> are not aligned to <code>PMM::PAGE_SIZE</code>,
 * the pages they land on are ignored. This is to prevent freeing partially used pages.
 * @param base Base address to start the block from
 * @param length Length of block
 * @see PMM::PAGE_SIZE, mark_region_used
 */
void mark_region_free(const uint64_t base, const uint64_t length)
{
    const uint64_t start_page = (base + PMM::PAGE_SIZE - 1) / PMM::PAGE_SIZE;
    const uint64_t end_page = (base + length) / PMM::PAGE_SIZE;

    for (uint64_t page = start_page; page < end_page && page < total_pages; page++)
    {
        if (bitmap_test(page))
        {
            bitmap_clear(page);
            used_pages--;
        }
    }
}

/**
 * Marks a memory region as used.
 * Aligned widely, so that if <code>base</code> or <code>base + length</code> are not aligned to <code>PMM::PAGE_SIZE</code>,
 * the pages they land on are included. This is to prevent partially used pages not being claimed.
 * @param base Base address to start the block from
 * @param length Length of block
 * @see PMM::PAGE_SIZE, mark_region_free
 */
void mark_region_used(const uint64_t base, const uint64_t length)
{
    const uint64_t start_page = base / PMM::PAGE_SIZE;
    const uint64_t end_page = (base + length + PMM::PAGE_SIZE - 1) / PMM::PAGE_SIZE;

    for (uint64_t page = start_page; page < end_page && page < total_pages; page++)
    {
        if (!bitmap_test(page))
        {
            bitmap_set(page);
            used_pages++;
        }
    }
}
}

namespace PMM
{
void init()
{
    const auto* entries = reinterpret_cast<const E820Entry*>(BOOT_INFO->mmap_addr);
    const uint32_t count = BOOT_INFO->mmap_entries;

    uint64_t highest_addr = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (entries[i].type == 1)
        {
            const uint64_t top = entries[i].base + entries[i].length;
            if (top > highest_addr)
                highest_addr = top;
        }
    }

    total_pages = highest_addr / PAGE_SIZE;
    used_pages = total_pages;
    const uint64_t bitmap_size = (total_pages + 7) / 8;

    const auto k_end = reinterpret_cast<uint64_t>(kernel_end);
    for (uint64_t i = 0; i < count; i++)
    {
        if (entries[i].type != 1) continue;

        uint64_t region_start = entries[i].base;
        const uint64_t region_end = entries[i].base + entries[i].length;

        if (region_start < k_end)
            region_start = k_end;

        region_start = (region_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        if (region_end > region_start && (region_end - region_start) >= bitmap_size)
        {
            bitmap = reinterpret_cast<uint8_t*>(region_start);
            break;
        }
    }

    for (uint64_t i = 0; i < bitmap_size; i++) { bitmap[i] = 0xFF; }

    for (uint64_t i = 0; i < count; i++)
    {
        if (entries[i].type == 1) { mark_region_free(entries[i].base, entries[i].length); }
    }

    mark_region_used(0, 0x100000);
    const auto k_start = reinterpret_cast<uint64_t>(kernel_start);
    mark_region_used(k_start, k_end - k_start);
    mark_region_used(reinterpret_cast<uint64_t>(bitmap), bitmap_size);

    Serial::print("[PMM] Initialized. Free RAM (bytes): ");
    Serial::print_hex(get_free_memory());
    Serial::print("\n");
}

uint64_t alloc_frame()
{
    for (uint64_t i = last_free_page; i < total_pages; i++)
    {
        if (!bitmap_test(i))
        {
            bitmap_set(i);
            used_pages++;
            last_free_page = i + 1;
            return i * PAGE_SIZE;
        }
    }

    for (uint64_t i = 0; i < last_free_page; i++)
    {
        if (!bitmap_test(i))
        {
            bitmap_set(i);
            used_pages++;
            last_free_page = i;
            return i * PAGE_SIZE;
        }
    }

    return 0;
}

void free_frame(const uint64_t phys_addr)
{
    const uint64_t page = phys_addr / PAGE_SIZE;
    if (page >= total_pages || !bitmap_test(page)) return;

    bitmap_clear(page);
    used_pages--;
    if (page < last_free_page)
        last_free_page = page;
}

uint64_t get_total_memory() { return total_pages * PAGE_SIZE; }

uint64_t get_used_memory() { return used_pages * PAGE_SIZE; }

uint64_t get_free_memory() { return (total_pages - used_pages) * PAGE_SIZE; }
}
