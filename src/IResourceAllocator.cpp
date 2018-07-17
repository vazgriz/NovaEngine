#include "NovaEngine/IResourceAllocator.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
Resource<T, TCreateInfo>::Resource(IResourceAllocator<T, TCreateInfo>& allocator, RawResource<T>& resource) {
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
    return *this;
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

template class Resource<vk::Buffer, vk::BufferCreateInfo>;
template class Resource<vk::Image, vk::ImageCreateInfo>;