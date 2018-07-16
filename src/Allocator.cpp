#include "NovaEngine/Allocator.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::Resource(Allocator<T, TCreateInfo>& allocator, RawResource<T>& resource) {
    m_allocator = &allocator;
    m_resource = &resource;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::Resource(Resource<T, TCreateInfo>&& other) {
    m_resource = other.m_resource;
    other.m_resource = nullptr;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>& Resource<T, TCreateInfo>::operator = (Resource<T, TCreateInfo>&& other) {
    free();
    m_resource = other.m_resource;
    other.m_resource = nullptr;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::~Resource() {
    free();
}

template<typename T, typename TCreateInfo>
void Resource<T, TCreateInfo>::free() {
    m_allocator->free(m_resource);
}

template<typename T, typename TCreateInfo>
void Resource<T, TCreateInfo>::registerUsage(size_t frame) {
    m_allocator->registerUsage(m_resource, frame);
}

template<typename T, typename TCreateInfo>
Allocator<T, TCreateInfo>::Allocator(Engine& engine, QueueGraph& queueGraph, size_t pageSize): m_allocator(engine, pageSize) {
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

    for (auto it = m_dead.begin(); it != m_dead.end(); it++) {
        if ((*it)->usage <= completed) {
            m_resources.erase((*it)->resource.get());
            m_dead.erase(it);
        }
    }
}

template class Allocator<vk::Buffer, vk::BufferCreateInfo>;
template class Allocator<vk::Image, vk::ImageCreateInfo>;