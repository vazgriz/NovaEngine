#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <unordered_set>
#include "NovaEngine/IAllocator.h"
#include "NovaEngine/FreeListAllocator.h"

namespace Nova {
    class Engine;
    class IResourceAllocatorBase;

    struct MemoryAllocation;

    class Memory {
    public:
        class Page {
        public:
            Page(vk::Device& device, uint32_t type, size_t size);
            Page(const Page& other) = delete;
            Page& operator = (const Page& other) = delete;
            Page(Page&& other) = default;
            Page& operator = (Page&& other) = default;

            vk::DeviceMemory& memory() const { return *m_memory; }
            vk::MemoryPropertyFlags flags() const { return m_flags; }
            void* mapping() const { return m_mapping; }

            MemoryAllocation tryAllocate(size_t size);
            void free(MemoryAllocation allocation);

        private:
            std::unique_ptr<vk::DeviceMemory> m_memory;
            std::unique_ptr<FreeListAllocator> m_allocator;
            vk::MemoryPropertyFlags m_flags;
            size_t m_size;
            size_t m_alignment;
            void* m_mapping = nullptr;
        };

        Memory(Engine& engine);
        Memory(const Memory& other) = delete;
        Memory& operator = (const Memory& other) = delete;
        Memory(Memory&& other) = default;
        Memory& operator = (Memory&& other) = default;

        const vk::MemoryProperties& properties() const { return m_properties; }

        MemoryAllocation allocate(uint32_t type, size_t size);
        void free(MemoryAllocation allocation);

        void addResourceAllocator(IResourceAllocatorBase& allocator);
        void removeResourceAllocator(IResourceAllocatorBase& allocator);
        void update(size_t completed);

    private:
        Engine* m_engine;
        vk::MemoryProperties m_properties;
        std::vector<std::vector<std::unique_ptr<Page>>> m_pages;
        std::unordered_set<IResourceAllocatorBase*> m_resourceAllocators;
    };
    
    struct MemoryAllocation {
        Memory::Page* memory;
        size_t offset;
        size_t size;
    };
}