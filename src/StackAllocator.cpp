#include "NovaEngine/StackAllocator.h"

using namespace Nova;

StackAllocator::StackAllocator(size_t offset, size_t size) {
    m_offset = offset;
    m_size = size;
    m_ptr = offset;
}

Allocation StackAllocator::allocate(size_t size, size_t alignment) {
    size_t ptr = IAllocator::align(m_ptr, alignment);
    size_t end = ptr + size;
    if (end <= m_offset + m_size) {
        m_stack.push_back(m_ptr);
        m_ptr = end;
        return { this, ptr, size };
    } else {
        return {};
    }
}

void StackAllocator::free(Allocation allocation) {
    if (allocation.allocator == nullptr) return;
    m_ptr = m_stack.back();
    m_stack.pop_back();
}

void StackAllocator::reset() {
    m_stack.clear();
    m_ptr = m_offset;
}