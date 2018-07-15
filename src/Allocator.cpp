#include "NovaEngine/Allocator.h"

using namespace Nova;

template<typename T, typename TCreateInfo>
Allocator<T, TCreateInfo>::Allocator(Engine& engine, size_t pageSize): m_allocator(engine, pageSize) {
    m_engine = &engine;
}

template class Allocator<vk::Buffer, vk::BufferCreateInfo>;
template class Allocator<vk::Image, vk::ImageCreateInfo>;