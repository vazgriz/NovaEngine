#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"
#include "NovaEngine/FreeListAllocator.h"

namespace Nova {
    class Engine;

    struct MemoryAllocation;

    class Memory {
    public:
        class Page {
        public:
            Page(vk::Device& device, uint32_t type, size_t size);

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

    private:
        Engine* m_engine;
        vk::MemoryProperties m_properties;
        std::vector<std::vector<Page>> m_pages;
    };
    
    struct MemoryAllocation {
        Memory::Page* memory;
        size_t offset;
        size_t size;
    };
}