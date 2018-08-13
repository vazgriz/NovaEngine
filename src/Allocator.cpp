#include "NovaEngine/Allocator.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
Allocator<T, TCreateInfo>::Allocator(Engine& engine, size_t pageSize) : IResourceAllocator(engine), m_allocator(engine, pageSize) {
    m_engine = &engine;
    m_dead.resize(m_engine->frameGraph().frameCount());
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo> Allocator<T, TCreateInfo>::allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    std::unique_ptr<RawResource<T>> raw = std::make_unique<RawResource<T>>(m_allocator.allocate(info, required, preferred));
    RawResource<T>* ptr = raw.get();
    m_resources[ptr] = std::move(raw);
    return Resource<T, TCreateInfo>(*this, *ptr);
}

template<typename T, typename TCreateInfo>
void Allocator<T, TCreateInfo>::free(RawResource<T>* resource) {
    size_t frame = m_engine->frameGraph().frame() % m_engine->frameGraph().frameCount();
    m_dead[frame].emplace_back(std::move(m_resources[resource]));
}

template<typename T, typename TCreateInfo>
void Allocator<T, TCreateInfo>::update(size_t completed) {
    size_t frame = m_engine->frameGraph().frame() % m_engine->frameGraph().frameCount();
    m_dead[frame].clear();
}

template class Allocator<vk::Buffer, vk::BufferCreateInfo>;
template class Allocator<vk::Image, vk::ImageCreateInfo>;