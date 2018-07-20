#pragma once
#include "NovaEngine/IRawAllocator.h"
#include "NovaEngine/Memory.h"

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
        Memory::Page& page() const { return *m_resource->page; }
        size_t size() const { return m_resource->allocation.size; }
        size_t offset() const { return m_resource->allocation.offset; }
        void registerUsage(size_t frame) const;

    private:
        IResourceAllocator<T, TCreateInfo>* m_allocator;
        RawResource<T>* m_resource;

        void free();
    };


    template<typename T, typename TCreateInfo>
    class IResourceAllocator {
    public:
        virtual Resource<T, TCreateInfo> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
        virtual void update(size_t completed) = 0;
        virtual void registerUsage(RawResource<T>* resource, size_t frame) = 0;
        virtual void free(RawResource<T>* resource) = 0;
    };

    using Buffer = Resource<vk::Buffer, vk::BufferCreateInfo>;
    using Image = Resource<vk::Image, vk::ImageCreateInfo>;
}