#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/Engine.h>
#include <NovaEngine/RawAllocator.h>

namespace Nova {
    template<typename T, typename TCreateInfo>
    class Allocator {
    public:
        Allocator(Engine& engine, size_t pageSize);
        Allocator(const Allocator& other) = delete;
        Allocator& operator = (const Allocator& other) = delete;
        Allocator(Allocator&& other) = default;
        Allocator& operator = (Allocator&& other) = default;

    private:
        Engine* m_engine;
        RawAllocator<T, TCreateInfo> m_allocator;
    };

    using BufferAllocator = Allocator<vk::Buffer, vk::BufferCreateInfo>;
    using ImageAllocator = Allocator<vk::Image, vk::ImageCreateInfo>;
}