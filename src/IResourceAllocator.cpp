#include "NovaEngine/IResourceAllocator.h"

using namespace Nova;

template<typename T>
Resource<T>::~Resource() {
    if (allocation.allocator != nullptr) {
        allocation.allocator->free(allocation);
    }
}