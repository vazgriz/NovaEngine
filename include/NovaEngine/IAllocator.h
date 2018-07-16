#pragma once

namespace Nova {
    class IAllocator;

    struct Allocation {
        IAllocator* allocator;
        size_t offset;
        size_t size;
    };

    class IAllocator {
    public:
        virtual Allocation allocate(size_t size, size_t alignment) = 0;
        virtual void free(Allocation allocation) = 0;
        virtual void reset() = 0;

        static size_t align(size_t ptr, size_t alignment);
    };
}