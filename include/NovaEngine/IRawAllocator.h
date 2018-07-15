#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"

namespace Nova {
    template <typename T>
    class RawResource {
    public:
        RawResource(const RawResource& other) = delete;
        RawResource& operator = (const RawResource& other) = delete;
        RawResource(RawResource&& other) = default;
        RawResource& operator = (RawResource&& other) = default;
        ~RawResource();

        T resource;
        Allocation allocation;
    };

    template <typename T, typename TCreateInfo>
    class IRawAllocator {
    public:
        virtual RawResource<T> allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
    };
}