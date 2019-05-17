#pragma once
#include "NovaEngine/IGenericAllocator.h"
#include <vector>

namespace Nova {
    class StackAllocator : public IGenericAllocator {
    public:
        StackAllocator(size_t offset, size_t size);
        StackAllocator(const StackAllocator& other) = delete;
        StackAllocator& operator = (const StackAllocator& other) = delete;
        StackAllocator(StackAllocator&& other) = default;
        StackAllocator& operator = (StackAllocator&& other) = default;

        Allocation allocate(size_t size, size_t alignment) override;
        void free(Allocation allocation) override;
        void reset() override;

    private:
        size_t m_offset;
        size_t m_size;
        size_t m_ptr;
        std::vector<size_t> m_stack;
    };
}