#include "NovaEngine/Allocator.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
Allocator<T, TCreateInfo>::Allocator(Engine& engine, QueueGraph& queueGraph, size_t pageSize) : m_allocator(engine, pageSize) {
    m_engine = &engine;
    m_queueGraph = &queueGraph;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo> Allocator<T, TCreateInfo>::allocate(const TCreateInfo& info, vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags preferred) {
    std::unique_ptr<RawResource<T>> raw = std::make_unique<RawResource<T>>(m_allocator.allocate(info, required, preferred));
    RawResource<T>* ptr = raw.get();
    m_resources[ptr] = { std::move(raw) };
    return Resource<T, TCreateInfo>(*this, *ptr);
}

template<typename T, typename TCreateInfo>
void Allocator<T, TCreateInfo>::registerUsage(RawResource<T>* resource, size_t frame) {
    m_resources[resource].usage = frame;
}

template<typename T, typename TCreateInfo>
void Allocator<T, TCreateInfo>::free(RawResource<T>* resource) {
    m_resources[resource].dead = true;
    m_dead.insert(&m_resources[resource]);
}

template<typename T, typename TCreateInfo>
void Allocator<T, TCreateInfo>::update() {
    size_t completed = m_queueGraph->completedFrames();

    auto it = m_dead.begin();
    while (it != m_dead.end()) {
        if ((*it)->usage <= completed) {
            m_resources.erase((*it)->resource.get());
            it = m_dead.erase(it);
        } else {
            it++;
        }
    }
}

template class Allocator<vk::Buffer, vk::BufferCreateInfo>;
template class Allocator<vk::Image, vk::ImageCreateInfo>;