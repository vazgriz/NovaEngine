#include "NovaEngine/Memory.h"
#include "NovaEngine/Engine.h"
#include "NovaEngine/IResourceAllocator.h"

#define PAGE_SIZE (256 * 1024 * 1024)

using namespace Nova;

Memory::Page::Page(vk::Device& device, uint32_t type, size_t size) {
    vk::MemoryAllocateInfo info = {};
    info.allocationSize = size;
    info.memoryTypeIndex = type;
    m_alignment = device.physicalDevice().properties().limits.bufferImageGranularity;

    m_memory = std::make_unique<vk::DeviceMemory>(device, info);
    m_allocator = std::make_unique<FreeListAllocator>(0, PAGE_SIZE);

    m_size = size;

    m_flags = device.physicalDevice().memoryProperties().memoryTypes[type].propertyFlags;

    if ((m_flags & vk::MemoryPropertyFlags::HostVisible) == vk::MemoryPropertyFlags::HostVisible) {
        m_mapping = m_memory->map(0, size);
    }
}

MemoryAllocation Memory::Page::tryAllocate(size_t size) {
    Allocation allocation = m_allocator->allocate(size, m_alignment);

    if (allocation.allocator == nullptr) {
        return {};
    }

    return { this, allocation.offset, allocation.size };
}

void Memory::Page::free(MemoryAllocation allocation) {
    Allocation alloc = { m_allocator.get(), allocation.offset, allocation.size };
    m_allocator->free(alloc);
}

Memory::Memory(Engine& engine) {
    m_engine = &engine;
    m_properties = m_engine->renderer().device().physicalDevice().memoryProperties();
    m_pages.resize(m_properties.memoryTypes.size());
}

MemoryAllocation Memory::allocate(uint32_t type, size_t size) {
    if (size > PAGE_SIZE) throw std::runtime_error("Allocation too large");

    for (auto& page : m_pages[type]) {
        MemoryAllocation result = page->tryAllocate(size);
        if (result.memory != nullptr) {
            return result;
        }
    }

    m_pages[type].emplace_back(std::make_unique<Page>(m_engine->renderer().device(), type, PAGE_SIZE));

    return m_pages[type].back()->tryAllocate(size);
}

void Memory::free(MemoryAllocation allocation) {
    if (allocation.memory == nullptr) return;

    uint32_t type = allocation.memory->memory().typeIndex();
    for (auto& page : m_pages[type]) {
        if (page.get() == allocation.memory) {
            page->free(allocation);
        }
    }
}

void Memory::addResourceAllocator(IResourceAllocatorBase& allocator) {
    m_resourceAllocators.insert(&allocator);
}

void Memory::removeResourceAllocator(IResourceAllocatorBase& allocator) {
    m_resourceAllocators.erase(&allocator);
}

void Memory::update(size_t completed) {
    for (auto allocator : m_resourceAllocators) {
        allocator->update(completed);
    }
}