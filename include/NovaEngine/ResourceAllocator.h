#pragma once
#include "NovaEngine/Memory.h"
#include "NovaEngine/Engine.h"
#include "NovaEngine/IResourceAllocator.h"
#include "NovaEngine/FreeListAllocator.h"
#include <unordered_set>

namespace Nova {
    template<typename T, typename TCreateInfo>
    class ResourceAllocator : public IResourceAllocator<T, TCreateInfo> {
        class Page {
        public:
            Page(Memory& memory, MemoryAllocation allocation);
            Page(const Page& other) = delete;
            Page& operator = (const Page& other) = delete;
            Page(Page&& other) = default;
            Page& operator = (Page&& other) = default;
            ~Page();

            vk::DeviceMemory& memory() const { return *m_allocation.memory; }
            size_t offset() const { return m_allocation.offset; }
            size_t size() const { return m_allocation.size; }
            FreeListAllocator& allocator() const { return *m_allocator; }

        private:
            Memory* m_memory;
            MemoryAllocation m_allocation;
            std::unique_ptr<FreeListAllocator> m_allocator;
        };

    public:
        ResourceAllocator(Engine& engine, size_t pageSize);
        ResourceAllocator(const ResourceAllocator& other) = delete;
        ResourceAllocator& operator = (const ResourceAllocator& other) = delete;
        ResourceAllocator(ResourceAllocator&& other) = default;
        ResourceAllocator& operator = (ResourceAllocator&& other) = default;
    
        Resource<T>& allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) override;
        void free(Resource<T>& resource) override;
    
    private:
        Engine* m_engine;
        Memory* m_memory;
        size_t m_pageSize;

        std::vector<std::vector<Page>> m_pages;
        std::unordered_set<std::unique_ptr<Resource<T>>> m_resources;

        Allocation bind(T& resource, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred);
        Allocation tryBind(T& resource, vk::MemoryPropertyFlags flags);
    };

    using BufferAllocator = ResourceAllocator<vk::Buffer, vk::BufferCreateInfo>;
    using ImageAllocator = ResourceAllocator<vk::Image, vk::ImageCreateInfo>;
}