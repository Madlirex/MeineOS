#include "vmm.hpp"
#include "boot_info.hpp"
#include "serial.hpp"

namespace
{
constexpr uint64_t ADDR_MASK = 0x000FFFFFFFFFF000ULL;
constexpr uint64_t HUGE_2MB = 0x200000ULL;

uint64_t kernel_pml4_phys = 0;

uint64_t alloc_zeroed_table()
{
    const uint64_t phys = PMM::alloc_frame();
    if (phys == 0) return 0;

    auto* ptr = static_cast<uint64_t*>(VMM::phys_to_virt(phys));
    for (int i = 0; i < 512; i++) { ptr[i] = 0; }
    return phys;
}

uint64_t* get_or_create_table(uint64_t* parent_table, const uint64_t index, const uint64_t flags)
{
    if (!(parent_table[index] & VMM::PageFlags::PRESENT))
    {
        const uint64_t new_table_phys = alloc_zeroed_table();
        if (new_table_phys == 0)
            return nullptr;

        uint64_t dir_flags = VMM::PageFlags::PRESENT | VMM::PageFlags::WRITABLE;
        if (flags & VMM::PageFlags::USER)
            dir_flags |= VMM::PageFlags::USER;

        parent_table[index] = new_table_phys | dir_flags;
    }
    const uint64_t next_table_phys = parent_table[index] & ADDR_MASK;
    return static_cast<uint64_t*>(VMM::phys_to_virt(next_table_phys));
}

void map_huge_2mb(uint64_t* pml4_virt, const uint64_t virt_addr, const uint64_t phys_addr, const uint64_t flags)
{
    const uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    const uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;

    uint64_t* pdpt_table = get_or_create_table(pml4_virt, pml4_idx, flags);
    uint64_t* pd_table = get_or_create_table(pdpt_table, pdpt_idx, flags);

    pd_table[pd_idx] = (phys_addr & ~0x1FFFFFULL) | flags | VMM::HUGE_PAGE;
}
}

namespace VMM
{
void init()
{
    kernel_pml4_phys = PMM::alloc_frame(); // Allocate frame for pml4
    auto* pml4_early = reinterpret_cast<uint64_t*>(kernel_pml4_phys);
    for (int i = 0; i < 512; i++) // Clear pml4
        pml4_early[i] = 0;

    // Map first 32 physical MB to the same virtual addresses (no offset)
    // IT MAY SEEM THE 2ND LOOP DOES THE SAME THING, THEY DO NOT
    for (uint64_t addr = 0; addr < 32 * 1024 * 1024ULL; addr += HUGE_2MB)
    {
        const uint64_t pdpt_idx = (addr >> 30) & 0x1FF;
        const uint64_t pd_idx = (addr >> 21) & 0x1FF;

        if (!(pml4_early[0] & PRESENT))
        {
            const uint64_t pdpt_phys = PMM::alloc_frame();
            auto* pdpt = reinterpret_cast<uint64_t*>(pdpt_phys);

            for (int i = 0; i < 512; i++)
                pdpt[i] = 0;

            pml4_early[0] = pdpt_phys | PRESENT | WRITABLE;
        }

        auto* pdpt = reinterpret_cast<uint64_t*>(pml4_early[0] & ADDR_MASK);
        if (!(pdpt[pdpt_idx] & PRESENT))
        {
            const uint64_t pd_phys = PMM::alloc_frame();
            auto* pd = reinterpret_cast<uint64_t*>(pd_phys);

            for (int i = 0; i < 512; i++)
                pd[i] = 0;

            pdpt[pdpt_idx] = pd_phys | PRESENT | WRITABLE;
        }

        auto* pd = reinterpret_cast<uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);
        pd[pd_idx] = addr | PRESENT | WRITABLE | HUGE_PAGE;
    }

    // Get max physical memory (or 4GB, so drivers are discovered)
    uint64_t max_phys = PMM::get_total_memory();
    if (max_phys < 4ULL * 1024 * 1024 * 1024) { max_phys = 4ULL * 1024 * 1024 * 1024; }
    max_phys = (max_phys + HUGE_2MB - 1) & ~(HUGE_2MB - 1); // Align to 2MB

    // Map physical addresses to their virtual counterparts
    for (uint64_t addr = 0; addr < max_phys; addr += HUGE_2MB)
    {
        const uint64_t virt = addr + HHDM_OFFSET;
        const uint64_t pml4_idx = (virt >> 39) & 0x1FF;
        const uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
        const uint64_t pd_idx = (virt >> 21) & 0x1FF;

        if (!(pml4_early[pml4_idx] & PRESENT))
        {
            const uint64_t pdpt_phys = PMM::alloc_frame();
            auto* pdpt = reinterpret_cast<uint64_t*>(pdpt_phys);

            for (int i = 0; i < 512; i++)
                pdpt[i] = 0;

            pml4_early[pml4_idx] = pdpt_phys | PRESENT | WRITABLE;
        }

        auto* pdpt = reinterpret_cast<uint64_t*>(pml4_early[pml4_idx] & ADDR_MASK);
        if (!(pdpt[pdpt_idx] & PRESENT))
        {
            const uint64_t pd_phys = PMM::alloc_frame();
            auto* pd = reinterpret_cast<uint64_t*>(pd_phys);

            for (int i = 0; i < 512; i++)
                pd[i] = 0;

            pdpt[pdpt_idx] = pd_phys | PRESENT | WRITABLE;
        }

        auto* pd = reinterpret_cast<uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);
        pd[pd_idx] = addr | PRESENT | WRITABLE | HUGE_PAGE;
    }

    // Switch to the kernel's PML4 page table and flush non-global TLB entries
    asm volatile("mov %0, %%cr3" : : "r"(kernel_pml4_phys) : "memory");

    // Map framebuffer
    auto* pml4_virt = static_cast<uint64_t*>(phys_to_virt(kernel_pml4_phys));
    const uint64_t fb_base = BOOT_INFO->framebuffer & ~(HUGE_2MB - 1);
    for (uint64_t offset = 0; offset < 16 * 1024 * 1024ULL; offset += HUGE_2MB)
    {
        map_huge_2mb(pml4_virt, fb_base + offset, fb_base + offset, PRESENT | WRITABLE);
    }

    // Flush
    asm volatile("mov %0, %%cr3" : : "r"(kernel_pml4_phys) : "memory");

    Serial::print("[VMM] 4-level paging active. HHDM mapped up to: ");
    Serial::print_hex(max_phys);
    Serial::print("\n");
}

void map_page(const uint64_t virt_addr, const uint64_t phys_addr, const uint64_t flags, const uint64_t pml4_phys)
{
    auto* pml4 = static_cast<uint64_t*>(phys_to_virt(pml4_phys));

    const uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    const uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    const uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;

    uint64_t* pdpt = get_or_create_table(pml4, pml4_idx, flags);
    uint64_t* pd = get_or_create_table(pdpt, pdpt_idx, flags);
    uint64_t* pt = get_or_create_table(pd, pd_idx, flags);

    pt[pt_idx] = (phys_addr & ADDR_MASK) | flags | PRESENT;

    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory"); // Flush TLB for this virtual address
}

void map_page(const uint64_t virt_addr, const uint64_t phys_addr, const uint64_t flags)
{
    return map_page(virt_addr, phys_addr, flags, kernel_pml4_phys);
}

void unmap_page(const uint64_t virt_addr, const uint64_t pml4_phys)
{
    auto* pml4 = static_cast<uint64_t*>(phys_to_virt(pml4_phys));

    const uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    const uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    const uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PRESENT)) return;
    auto* pdpt = static_cast<uint64_t*>(phys_to_virt(pml4[pml4_idx] & ADDR_MASK));

    if (!(pdpt[pdpt_idx] & PRESENT)) return;
    auto* pd = static_cast<uint64_t*>(phys_to_virt(pdpt[pdpt_idx] & ADDR_MASK));

    if (!(pd[pd_idx] & PRESENT)) return;
    auto* pt = static_cast<uint64_t*>(phys_to_virt(pd[pd_idx] & ADDR_MASK));

    pt[pt_idx] = 0;
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory"); // Flush TLB for this virtual address
}

void unmap_page(const uint64_t virt_addr) { return unmap_page(virt_addr, kernel_pml4_phys); }

uint64_t virt_to_phys(const uint64_t virt_addr, const uint64_t pml4_phys)
{
    const auto* pml4 = static_cast<uint64_t*>(phys_to_virt(pml4_phys));

    const uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    const uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    const uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PRESENT)) return 0;
    const auto* pdpt = static_cast<uint64_t*>(phys_to_virt(pml4[pml4_idx] & ADDR_MASK));

    if (!(pdpt[pdpt_idx] & PRESENT)) return 0;
    if (pdpt[pdpt_idx] & HUGE_PAGE) // 1GB mapped
        return (pdpt[pdpt_idx] & 0x000FFFFFC0000000ULL) + (virt_addr & 0x3FFFFFFFULL);

    const auto* pd = static_cast<uint64_t*>(phys_to_virt(pdpt[pdpt_idx] & ADDR_MASK));
    if (!(pd[pd_idx] & PRESENT)) return 0;
    if (pd[pd_idx] & HUGE_PAGE) // 2MB mapped
        return (pd[pd_idx] & 0x000FFFFFFFE00000ULL) + (virt_addr & 0x1FFFFFULL);

    const auto* pt = static_cast<uint64_t*>(phys_to_virt(pd[pd_idx] & ADDR_MASK));
    if (!(pt[pt_idx] & PRESENT)) return 0;

    return (pt[pt_idx] & ADDR_MASK) + (virt_addr & 0xFFFULL);
}

uint64_t virt_to_phys(const uint64_t virt_addr) { return virt_to_phys(virt_addr, kernel_pml4_phys); }
}
