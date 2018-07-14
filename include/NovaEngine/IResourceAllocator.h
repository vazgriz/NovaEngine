#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"

namespace Nova {
    template <typename T>
    struct Resource {
        T resource;
        Allocation allocation;
    };

    template <typename T, typename TCreateInfo>
    class IResourceAllocator {
    public:
        virtual Resource<T>& allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
        virtual void free(Resource<T>& resource) = 0;
    };
}