#include "NovaEngine/StagingAllocator.h"

using namespace Nova;

StagingAllocator::StagingAllocator(Engine& engine, size_t pageSize, uint32_t type) {
    m_engine = &engine;
    m_memory = &engine.memory();

    m_page = m_memory->allocate(type, pageSize);
    m_allocator = std::make_unique<LinearAllocator>(m_page.offset, m_page.size);

    vk::BufferCreateInfo info = {};
    info.usage = vk::BufferUsageFlags::TransferSrc;
    info.size = pageSize;

    m_buffer = std::make_unique<vk::Buffer>(m_engine->renderer().device(), info);
    m_buffer->bind(m_page.memory->memory(), m_page.offset);
}

StagingAllocator::~StagingAllocator() {
    m_memory->free(m_page);
}

size_t StagingAllocator::stage(void* data, size_t size) {
    Allocation allocation = m_allocator->allocate(size, 4);
    void* dest = m_page.memory->mapping();
    dest = static_cast<void*>(static_cast<char*>(dest) + allocation.offset);
    memcpy(dest, data, size);
    return allocation.offset;
}

void StagingAllocator::reset() {
    m_allocator->reset();
}