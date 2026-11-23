#pragma once

#include <cstddef>

namespace Heap
{
void init();

void* kmalloc(std::size_t size);

void kfree(void* ptr);
}

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;