#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/RawAllocator.h"
#include "NovaEngine/IResourceAllocator.h"

namespace Nova {
    class Engine;

    template<typename T, typename TCreateInfo>
    class Allocator : public IResourceAllocator<T, TCreateInfo> {
        struct ResourceUsage {
            std::unique_ptr<RawResource<T>> resource;
            size_t usage = 0;
            bool dead = false;
        };

    public:
        Allocator(Engine& engine, size_t pageSize);
        Allocator(const Allocator& other) = delete;
        Allocator& operator = (const Allocator& other) = delete;
        Allocator(Allocator&& other) = default;
        Allocator& operator = (Allocator&& other) = default;

        Resource<T, TCreateInfo> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) override;
        void update(size_t completed) override;
        void registerUsage(RawResource<T>* resource, size_t frame) override;
        void free(RawResource<T>* resource) override;

    private:
        RawAllocator<T, TCreateInfo> m_allocator;
        std::unordered_map<RawResource<T>*, ResourceUsage> m_resources;
        std::unordered_set<ResourceUsage*> m_dead;
    };

    using BufferAllocator = Allocator<vk::Buffer, vk::BufferCreateInfo>;
    using ImageAllocator = Allocator<vk::Image, vk::ImageCreateInfo>;
}