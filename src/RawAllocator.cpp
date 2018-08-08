#include "NovaEngine/RawAllocator.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
RawAllocator<T, TCreateInfo>::Page::Page(Memory& memory, MemoryAllocation allocation) {
    m_memory = &memory;
    m_allocation = allocation;
    m_allocator = std::make_unique<FreeListAllocator>(allocation.offset, allocation.size);
}

template<typename T, typename TCreateInfo>
RawAllocator<T, TCreateInfo>::Page::~Page() {
    m_memory->free(m_allocation);
}

template<typename T, typename TCreateInfo>
RawAllocator<T, TCreateInfo>::RawAllocator(Engine& engine, size_t pageSize) {
    m_engine = &engine;
    m_memory = &engine.memory();
    m_pageSize = pageSize;

    m_pages.resize(m_memory->properties().memoryTypes.size());
}

template<typename T, typename TCreateInfo>
RawResource<T> RawAllocator<T, TCreateInfo>::allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    RawResource<T> resource = RawResource<T>(T(m_engine->renderer().device(), info));

    BindResult result = bind(resource.resource, required, preferred);
    resource.allocation = result.allocation;
    resource.page = result.page;

    return resource;
}

template<typename T, typename TCreateInfo>
typename RawAllocator<T, TCreateInfo>::BindResult RawAllocator<T, TCreateInfo>::bind(T& resource, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    BindResult result = tryBind(resource, required | preferred);
    if (result.allocation.allocator != nullptr) {
        return result;
    }

    result = tryBind(resource, required);
    if (result.allocation.allocator != nullptr) {
        return result;
    }

    return {};
}

template<typename T, typename TCreateInfo>
typename RawAllocator<T, TCreateInfo>::BindResult RawAllocator<T, TCreateInfo>::tryBind(T& resource, vk::MemoryPropertyFlags flags) {
    vk::MemoryRequirements requirements = resource.requirements();
    auto& types = m_memory->properties().memoryTypes;

    //check global memory properties
    for (uint32_t i = 0; i < types.size(); i++) {
        if ((requirements.memoryTypeBits & (1 << i)) != 0) {
            auto& type = types[i];
            if ((type.propertyFlags & flags) == flags) {
                //check local pages for free space
                for (auto& page : m_pages[i]) {
                    Allocation allocation = page.allocator().allocate(requirements.size, requirements.alignment);
                    if (allocation.allocator != nullptr) {
                        resource.bind(page.memory().memory(), allocation.offset);
                        return { allocation, &page.memory() };
                    }
                }

                //create more local pages
                MemoryAllocation memoryAllocation = m_memory->allocate(i, m_pageSize);
                if (memoryAllocation.memory != nullptr) {
                    m_pages[i].emplace_back(*m_memory, memoryAllocation);
                    Page& newPage = m_pages[i].back();
                    Allocation allocation = newPage.allocator().allocate(requirements.size, requirements.alignment);
                    if (allocation.allocator != nullptr) {
                        resource.bind(newPage.memory().memory(), allocation.offset);
                        return { allocation, &newPage.memory() };
                    }
                }
            }
        }
    }

    return {};
}

template class RawAllocator<vk::Buffer, vk::BufferCreateInfo>;
template class RawAllocator<vk::Image, vk::ImageCreateInfo>;