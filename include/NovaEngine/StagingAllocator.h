#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Memory.h"
#include "NovaEngine/LinearAllocator.h"

namespace Nova {
    class Engine;

    class StagingAllocator {
    public:
        StagingAllocator(Engine& engine, size_t pageSize, uint32_t type);
        StagingAllocator(const StagingAllocator& other) = delete;
        StagingAllocator& operator = (const StagingAllocator& other) = delete;
        StagingAllocator(StagingAllocator&& other);
        StagingAllocator& operator = (StagingAllocator&& other);
        ~StagingAllocator();

        vk::Buffer& buffer() const { return *m_buffer; }

        size_t stage(const void* data, size_t size);
        void reset();

    private:
        Engine* m_engine;
        Memory* m_memory;
        std::unique_ptr<LinearAllocator> m_allocator;
        std::unique_ptr<vk::Buffer> m_buffer;
        MemoryAllocation m_page = {};
    };
}