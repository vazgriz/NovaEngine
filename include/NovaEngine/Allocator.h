#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/RawAllocator.h"
#include "NovaEngine/IResourceAllocator.h"

namespace Nova {
    class Engine;

    template<typename T, typename TCreateInfo>
    class Allocator : public IResourceAllocator<T, TCreateInfo> {
    public:
        Allocator(Engine& engine, size_t pageSize);
        Allocator(const Allocator& other) = delete;
        Allocator& operator = (const Allocator& other) = delete;
        Allocator(Allocator&& other) = default;
        Allocator& operator = (Allocator&& other) = default;

        Resource<T, TCreateInfo> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) override;
        void update(size_t completed) override;
        void free(RawResource<T>* resource) override;

    private:
        Engine* m_engine;
        RawAllocator<T, TCreateInfo> m_allocator;
        std::unordered_map<RawResource<T>*, std::unique_ptr<RawResource<T>>> m_resources;
        std::vector<std::vector<std::unique_ptr<RawResource<T>>>> m_dead;
    };

    using BufferAllocator = Allocator<vk::Buffer, vk::BufferCreateInfo>;
    using ImageAllocator = Allocator<vk::Image, vk::ImageCreateInfo>;
}