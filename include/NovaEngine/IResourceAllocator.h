#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"

namespace Nova {
    template <typename T>
    class Resource {
    public:
        Resource(const Resource& other) = delete;
        Resource& operator = (const Resource& other) = delete;
        Resource(Resource&& other) = default;
        Resource& operator = (Resource&& other) = default;
        ~Resource();

        T resource;
        Allocation allocation;
    };

    template <typename T, typename TCreateInfo>
    class IResourceAllocator {
    public:
        virtual Resource<T> allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
    };
}