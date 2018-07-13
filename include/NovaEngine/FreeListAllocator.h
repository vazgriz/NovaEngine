#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"
#include <list>

namespace Nova {
    class FreeListAllocator : public IAllocator {
        struct Node {
            size_t offset;
            size_t size;
        };

    public:
        FreeListAllocator(size_t offset, size_t size);
        FreeListAllocator(const FreeListAllocator& other) = delete;
        FreeListAllocator& operator = (const FreeListAllocator& other) = delete;
        FreeListAllocator(FreeListAllocator&& other) = default;
        FreeListAllocator& operator = (FreeListAllocator&& other) = default;

        Allocation allocate(size_t size, size_t alignment) override;
        void free(Allocation allocation) override;
        void reset() override;

    private:
        size_t m_offset;
        size_t m_size;
        std::list<Node> m_nodes;

        void split(std::list<Node>::iterator it, size_t offset, size_t size);
        void merge(std::list<Node>::iterator it, Allocation allocation);
        void merge(std::list<Node>::iterator front, std::list<Node>::iterator back);
    };
}