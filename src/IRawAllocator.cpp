#include "NovaEngine/IRawAllocator.h"

using namespace Nova;

template<typename T>
RawResource<T>::~RawResource() {
    if (allocation.allocator != nullptr) {
        allocation.allocator->free(allocation);
    }
}