#include "NovaEngine/IRawAllocator.h"

using namespace Nova;

template<typename T>
RawResource<T>::RawResource(T&& resource) : resource(std::move(resource)) {

}

template<typename T>
RawResource<T>::RawResource(RawResource&& other) : resource(std::move(other.resource)) {
    allocation = other.allocation;
    other.allocation.allocator = nullptr;
    page = other.page;
}

template<typename T>
RawResource<T>& RawResource<T>::operator = (RawResource&& other) {
    if (allocation.allocator != nullptr) {
        allocation.allocator->free(allocation);
    }
    resource = std::move(other.resource);
    allocation = other.allocation;
    other.allocation.allocator = nullptr;
    page = other.page;
    return *this;
}

template<typename T>
RawResource<T>::~RawResource() {
    if (allocation.allocator != nullptr) {
        allocation.allocator->free(allocation);
    }
}

template class RawResource<vk::Buffer>;
template class RawResource<vk::Image>;