#include "NovaEngine/GenericAllocator.h"

using namespace Nova;

size_t GenericAllocator::align(size_t ptr, size_t alignment) {
    size_t unalign = ptr % alignment;

    if (unalign == 0) {
        return ptr;
    } else {
        size_t align = alignment - unalign;
        return ptr + align;
    }
}