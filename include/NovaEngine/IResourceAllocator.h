#pragma once
#include "NovaEngine/IRawAllocator.h"

namespace Nova {
    template<typename T, typename TCreateInfo>
    class IResourceAllocator;

    template<typename T, typename TCreateInfo>
    class Resource {
    public:
        Resource(IResourceAllocator<T, TCreateInfo>& allocator, RawResource<T>& resource);
        Resource(const Resource<T, TCreateInfo>& other) = delete;
        Resource<T, TCreateInfo>& operator = (const Resource<T, TCreateInfo>& other) = delete;
        Resource(Resource<T, TCreateInfo>&& other);
        Resource<T, TCreateInfo>& operator = (Resource<T, TCreateInfo>&& other);
        ~Resource();

        T& resource() const { return m_resource->resource; }
        void registerUsage(size_t frame);

    private:
        IResourceAllocator<T, TCreateInfo>* m_allocator;
        RawResource<T>* m_resource;

        void free();
    };


    template<typename T, typename TCreateInfo>
    class IResourceAllocator {
    public:
        virtual Resource<T, TCreateInfo> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
        virtual void update() = 0;
        virtual void registerUsage(RawResource<T>* resource, size_t frame) = 0;
        virtual void free(RawResource<T>* resource) = 0;
    };

    using Buffer = Resource<vk::Buffer, vk::BufferCreateInfo>;
    using Image = Resource<vk::Image, vk::ImageCreateInfo>;
}