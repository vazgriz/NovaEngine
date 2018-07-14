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
Resource<T>& ResourceAllocator<T, TCreateInfo>::allocate(TCreateInfo info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    std::unique_ptr<Resource<T>> resource = std::make_unique<Resource<T>>(Resource<T>{ T(m_engine->renderer().device(), info) });

    Allocation allocation = bind(resource->resource, required, preferred);
    resource->allocation = allocation;

    auto result = m_resources.emplace(std::move(resource));
    return **result.first;
}

template<typename T, typename TCreateInfo>
void ResourceAllocator<T, TCreateInfo>::free(Resource<T>& resource) {
    std::unordered_set<std::unique_ptr<Resource<T>>>::iterator it = std::find_if(m_resources.begin(), m_resources.end(), [&](const std::unique_ptr<Resource<T>>& ptr) {
        return &resource == ptr.get();
    });

    if (it != m_resources.end()) {
        resource.allocation.allocator->free(resource.allocation);
        m_resources.erase(it);
    }
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