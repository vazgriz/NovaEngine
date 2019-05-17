#pragma once
#include "NovaEngine/IGenericAllocator.h"

namespace Nova {
    class LinearAllocator : public IGenericAllocator {
    public:
        LinearAllocator(size_t offset, size_t size);
        LinearAllocator(const LinearAllocator& other) = delete;
        LinearAllocator& operator = (const LinearAllocator& other) = delete;
        LinearAllocator(LinearAllocator&& other) = default;
        LinearAllocator& operator = (LinearAllocator&& other) = default;

        Allocation allocate(size_t size, size_t alignment) override;
        void free(Allocation allocation) override;
        void reset() override;

    private:
        size_t m_offset;
        size_t m_size;
        size_t m_ptr;
    };
}