#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/IAllocator.h"
#include "NovaEngine/Memory.h"

namespace Nova {
    template <typename T>
    class RawResource {
    public:
        RawResource(T&& resource);
        RawResource(const RawResource& other) = delete;
        RawResource& operator = (const RawResource& other) = delete;
        RawResource(RawResource&& other);
        RawResource& operator = (RawResource&& other);
        ~RawResource();

        T resource;
        Allocation allocation;
        Memory::Page* page;
    };

    template <typename T, typename TCreateInfo>
    class IRawAllocator {
    public:
        virtual RawResource<T> allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) = 0;
    };
}