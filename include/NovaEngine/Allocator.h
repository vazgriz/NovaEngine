#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/Engine.h>
#include <NovaEngine/QueueGraph.h>
#include <NovaEngine/RawAllocator.h>

namespace Nova {
    template<typename T, typename TCreateInfo>
    class Allocator;

    template<typename T, typename TCreateInfo>
    class Resource {
    public:
        Resource(Allocator<T, TCreateInfo>& allocator, RawResource<T>& resource);
        Resource(const Resource<T, TCreateInfo>& other) = delete;
        Resource<T, TCreateInfo>& operator = (const Resource<T, TCreateInfo>& other) = delete;
        Resource(Resource<T, TCreateInfo>&& other);
        Resource<T, TCreateInfo>& operator = (Resource<T, TCreateInfo>&& other);
        ~Resource();

        T& resource() const { return m_resource->resource; }
        void registerUsage(size_t frame);

    private:
        Allocator<T, TCreateInfo>* m_allocator;
        RawResource<T>* m_resource;

        void free();
    };

    template<typename T, typename TCreateInfo>
    class Allocator {
        friend Resource<T, TCreateInfo>;

        struct ResourceUsage {
            std::unique_ptr<RawResource<T>> resource;
            size_t usage = 0;
            bool dead = false;
        };

    public:
        Allocator(Engine& engine, QueueGraph& queueGraph, size_t pageSize);
        Allocator(const Allocator& other) = delete;
        Allocator& operator = (const Allocator& other) = delete;
        Allocator(Allocator&& other) = default;
        Allocator& operator = (Allocator&& other) = default;

        Resource<T, TCreateInfo> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred);
        void update();

    private:
        Engine* m_engine;
        QueueGraph* m_queueGraph;
        RawAllocator<T, TCreateInfo> m_allocator;
        std::unordered_map<RawResource<T>*, ResourceUsage> m_resources;
        std::unordered_set<ResourceUsage*> m_dead;

        void registerUsage(RawResource<T>* resource, size_t frame);
        void free(RawResource<T>* resource);
    };

    using BufferAllocator = Allocator<vk::Buffer, vk::BufferCreateInfo>;
    using ImageAllocator = Allocator<vk::Image, vk::ImageCreateInfo>;
    using Buffer = Resource<vk::Buffer, vk::BufferCreateInfo>;
    using Image = Resource<vk::Image, vk::ImageCreateInfo>;
}