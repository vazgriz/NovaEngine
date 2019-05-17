#include "NovaEngine/LinearAllocator.h"

using namespace Nova;

LinearAllocator::LinearAllocator(size_t offset, size_t size) {
    m_offset = offset;
    m_size = size;
    m_ptr = offset;
}

Allocation LinearAllocator::allocate(size_t size, size_t alignment) {
    size_t ptr = IGenericAllocator::align(m_ptr, alignment);
    size_t end = ptr + size;
    if (end <= m_offset + m_size) {
        m_ptr = end;
        return { this, ptr, size };
    } else {
        return {};
    }
}

void LinearAllocator::free(Allocation allocation) {
    //nothing
}

void LinearAllocator::reset() {
    m_ptr = m_offset;
}