#include "heap.hpp"
#include "pmm.hpp"
#include "vmm.hpp"
#include "serial.hpp"

namespace
{
constexpr uint64_t HEAP_START = 0xFFFF900000000000ULL;
constexpr uint64_t HEAP_MAX = 0xFFFF900040000000ULL;
constexpr size_t INITIAL_PAGES = 16;

struct alignas(16) BlockHeader
{
    size_t size;
    bool is_free;
    BlockHeader* next;
    BlockHeader* prev;
};

uint64_t heap_end = HEAP_START;
BlockHeader* head = nullptr;

inline size_t align_up(const size_t val, const size_t align)
{
    return (val + align - 1) & ~(align - 1);
}

bool expand_heap(const size_t page_count)
{
    if (heap_end + page_count * VMM::PAGE_SIZE > HEAP_MAX) return false;

    const uint64_t old_end = heap_end;

    for (size_t i = 0; i < page_count; i++)
    {
        const uint64_t phys = PMM::alloc_frame();
        if (phys == 0) return false;

        VMM::map_page(heap_end, phys, VMM::WRITABLE);
        heap_end += VMM::PAGE_SIZE;
    }

    const size_t added_bytes = page_count * VMM::PAGE_SIZE;
    auto* new_block = reinterpret_cast<BlockHeader*>(old_end);

    new_block->size = added_bytes - sizeof(BlockHeader);
    new_block->is_free = true;
    new_block->next = nullptr;
    new_block->prev = nullptr;

    if (!head)
    {
        head = new_block;
        return true;
    }

    BlockHeader* tail = head;
    while (tail->next) tail = tail->next;

    if (tail->is_free)
    {
        tail->size += added_bytes;
    }
    else
    {
        tail->next = new_block;
        new_block->prev = tail;
    }

    return true;
}

}

namespace Heap
{
void init()
{
    if (!expand_heap(INITIAL_PAGES))
    {
        Serial::print("[Heap] PANIC: Failed to allocate initial heap!\n");
        return;
    }
    Serial::print("[Heap] Initialized at:" );
    Serial::print_hex(HEAP_START);
    Serial::print("\n");
}

void* kmalloc(std::size_t size)
{
    if (size == 0) return nullptr;
    size = align_up(size, 16);

    BlockHeader* curr = head;
    while (curr)
    {
        if (curr->is_free && curr->size >= size)
        {
            if (curr->size >= size + sizeof(BlockHeader) + 16)
            {
                auto* new_block = reinterpret_cast<BlockHeader*>(reinterpret_cast<uint8_t*>(curr) + sizeof(BlockHeader) + size);

                new_block->size = curr->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = curr->next;
                new_block->prev = curr;

                if (curr->next) curr->next->prev = new_block;
                curr->next = new_block;
                curr->size = size;
            }

            curr->is_free = false;
            return reinterpret_cast<uint8_t*>(curr) + sizeof(BlockHeader);
        }

        if (!curr->next)
        {
            const size_t needed = size + sizeof(BlockHeader);
            const size_t pages = (needed + VMM::PAGE_SIZE - 1) / VMM::PAGE_SIZE;
            if (!expand_heap(pages < 4 ? 4 : pages)) return nullptr;
        }
        else
        {
            curr = curr->next;
        }
    }

    return nullptr;
}

void kfree(void* ptr)
{
    if (!ptr) return;

    auto* block = reinterpret_cast<BlockHeader*>(static_cast<uint8_t*>(ptr) - sizeof(BlockHeader));
    block->is_free = true;

    if (block->next && block->next->is_free)
    {
        block->size += sizeof(BlockHeader) + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }

    if (block->prev && block->prev->is_free)
    {
        block->prev->size = sizeof(BlockHeader) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}
}

void* operator new(const size_t size) { return Heap::kmalloc(size); }
void* operator new [](const size_t size) { return Heap::kmalloc(size); }
void operator delete(void* ptr) noexcept { Heap::kfree(ptr); }
void operator delete[](void* ptr) noexcept { Heap::kfree(ptr); }
void operator delete(void* ptr, [[maybe_unused]] const size_t size) noexcept { Heap::kfree(ptr); }
void operator delete[](void* ptr, [[maybe_unused]] const size_t size) noexcept { Heap::kfree(ptr); }
