#include "NovaEngine/IResourceAllocator.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

IResourceAllocatorBase::IResourceAllocatorBase(Engine& engine) {
    m_engine = &engine;
    m_engine->memory().addResourceAllocator(*this);
}

IResourceAllocatorBase::~IResourceAllocatorBase() {
    m_engine->memory().removeResourceAllocator(*this);
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::Resource(IResourceAllocator<T, TCreateInfo>& allocator, RawResource<T>& resource) {
    m_allocator = &allocator;
    m_resource = &resource;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::Resource(Resource<T, TCreateInfo>&& other) {
    m_resource = other.m_resource;
    other.m_resource = nullptr;
    m_allocator = other.m_allocator;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>& Resource<T, TCreateInfo>::operator = (Resource<T, TCreateInfo>&& other) {
    free();
    m_resource = other.m_resource;
    other.m_resource = nullptr;
    m_allocator = other.m_allocator;
    return *this;
}

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::~Resource() {
    free();
}

template<typename T, typename TCreateInfo>
void Resource<T, TCreateInfo>::free() {
    if (m_resource == nullptr) return;
    m_allocator->free(m_resource);
}

template<typename T, typename TCreateInfo>
IResourceAllocator<T, TCreateInfo>::IResourceAllocator(Engine& engine) : IResourceAllocatorBase(engine) {

}

template class Resource<vk::Buffer, vk::BufferCreateInfo>;
template class Resource<vk::Image, vk::ImageCreateInfo>;
template class IResourceAllocator<vk::Buffer, vk::BufferCreateInfo>;
template class IResourceAllocator<vk::Image, vk::ImageCreateInfo>;