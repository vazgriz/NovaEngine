#include "NovaEngine/IRawAllocator.h"

using namespace Nova;

template<typename T>
RawResource<T>::~RawResource() {
    if (allocation.allocator != nullptr) {
        allocation.allocator->free(allocation);
    }
}

template class RawResource<vk::Buffer>;
template class RawResource<vk::Image>;