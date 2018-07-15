#include "NovaEngine/ResourceAllocator.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
ResourceAllocator<T, TCreateInfo>::Page::Page(Memory& memory, MemoryAllocation allocation) {
    m_memory = &memory;
    m_allocation = allocation;
    m_allocator = std::make_unique<FreeListAllocator>(allocation.offset, allocation.size);
}

template<typename T, typename TCreateInfo>
ResourceAllocator<T, TCreateInfo>::Page::~Page() {
    m_memory->free(m_allocation);
}

template<typename T, typename TCreateInfo>
ResourceAllocator<T, TCreateInfo>::ResourceAllocator(Engine& engine, size_t pageSize) {
    m_engine = &engine;
    m_memory = &engine.memory();
    m_pageSize = pageSize;

    m_pages.resize(m_memory->properties().memoryTypes.size());
}

template<typename T, typename TCreateInfo>
Resource<T> ResourceAllocator<T, TCreateInfo>::allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    Resource<T> resource = { T(m_engine->renderer().device(), info) };

    Allocation allocation = bind(resource.resource, required, preferred);
    resource.allocation = allocation;

    return resource;
}

template<typename T, typename TCreateInfo>
Allocation ResourceAllocator<T, TCreateInfo>::bind(T& resource, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    Allocation result = tryBind(resource, required | preferred);
    if (result.allocator != nullptr) {
        return result;
    }

    result = tryBind(resource, required);
    if (result.allocator != nullptr) {
        return result;
    }

    return {};
}

template<typename T, typename TCreateInfo>
Allocation ResourceAllocator<T, TCreateInfo>::tryBind(T& resource, vk::MemoryPropertyFlags flags) {
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
                        resource.bind(page.memory(), allocation.offset);
                        return allocation;
                    }
                }

                //create more local pages
                MemoryAllocation memoryAllocation = m_memory->allocate(i, m_pageSize);
                if (memoryAllocation.memory != nullptr) {
                    m_pages[i].emplace_back(*m_memory, memoryAllocation);
                    Page& newPage = m_pages[i].back();
                    Allocation allocation = newPage.allocator().allocate(requirements.size, requirements.alignment);
                    if (allocation.allocator != nullptr) {
                        resource.bind(newPage.memory(), allocation.offset);
                        return allocation;
                    }
                }
            }
        }
    }

    return {};
}

template class ResourceAllocator<vk::Buffer, vk::BufferCreateInfo>;
template class ResourceAllocator<vk::Image, vk::ImageCreateInfo>;