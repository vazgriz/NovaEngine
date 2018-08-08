#pragma once
#include <vector>
#include <stack>
#include <algorithm>
#include <utility>

namespace Nova {
    struct SlotID {
        uint32_t index;
        uint32_t version;
    };

    template<typename T>
    class SlotMap;

    template<typename T>
    class SlotHandle {
        friend class SlotMap<T>;

    public:
        SlotHandle() {}
        SlotHandle(const SlotHandle& other) = delete;
        SlotHandle& operator = (const SlotHandle& other) = delete;

        SlotHandle(SlotHandle&& other) {
            m_slotMap = other.m_slotMap;
            m_slotID = other.m_slotID;
            m_item = other.m_item;
            other.m_slotMap = nullptr;
        }

        SlotHandle& operator = (SlotHandle&& other) {
            if (m_slotMap != mullptr) {
                m_slotMap->destroy(m_slotID);
            }

            m_slotMap = other.m_slotMap;
            m_slotID = other.m_slotID;
            m_item = other.m_item;
            other.m_slotMap = nullptr;
        }

        ~SlotHandle() {
            if (m_slotMap != nullptr) {
                m_slotMap->destroy(m_slotID);
            }
        }

        SlotID id() const { return m_slotID; }

        T& operator * () const {
            return *m_item;
        }

        T& operator -> () const {
            return *m_item;
        }

    private:
        SlotMap<T>* m_slotMap = nullptr;
        SlotID m_slotID;
        T* m_item;

        SlotHandle(SlotMap<T>* slotMap, SlotID slotID) {
            m_slotMap = slotMap;
            m_slotID = slotID;
            m_item = &m_slotMap->get(m_slotID);
        }
    };

    template<typename T>
    class SlotMap {
        friend class SlotHandle<T>;

        static constexpr size_t chunkSize = std::max<size_t>(4096 / sizeof(T), 1);

        struct Chunk {
            std::unique_ptr<char[]> data;
            uint32_t version[chunkSize];
            bool live[chunkSize];

            void destroy(size_t index) {
                T* ptr = reinterpret_cast<T*>(data.get());
                ptr[index].~T();
                version[index]++;
                live[index] = false;
            }
        };

    public:
        SlotMap() {}
        SlotMap(const SlotMap& other) = delete;
        SlotMap& operator = (const SlotMap& other) = delete;
        SlotMap(SlotMap&& other) = default;
        SlotMap& operator = (SlotMap&& other) = default;

        ~SlotMap() {
            for (Chunk& chunk : m_chunks) {
                T* items = reinterpret_cast<T*>(chunk.data.get());
                for (size_t i = 0; i < chunkSize; i++) {
                    if (chunk.live[i]) {
                        items[i].~T();
                    }
                }
            }
        }

        template<typename... Args>
        SlotHandle<T> allocate(Args... args) {
            uint32_t virtualIndex;

            if (m_free.empty()) {
                m_chunks.emplace_back(Chunk{ std::make_unique<char[]>(chunkSize * sizeof(T)) });

                uint32_t startIndex = static_cast<uint32_t>((m_chunks.size() - 1) * chunkSize);
                for (uint32_t i = 0; i < chunkSize; i++) {
                    m_free.push(startIndex + (chunkSize - i - 1));
                }
            }

            virtualIndex = m_free.top();
            m_free.pop();

            uint32_t chunkIndex = virtualIndex / chunkSize;
            uint32_t itemIndex = virtualIndex % chunkSize;

            Chunk& chunk = m_chunks[chunkIndex];
            chunk.live[itemIndex] = true;
            T item = reinterpret_cast<T*>(chunk.data.get())[itemIndex];
            new (&item) T(std::forward<Args...>(args...));

            return SlotHandle<T>(this, { static_cast<uint32_t>(virtualIndex), chunk.version[itemIndex] });
        }

        T& get(SlotID slot) {
            uint32_t virtualIndex = slot.index;
            uint32_t version = slot.version;

            uint32_t chunkIndex = virtualIndex / chunkSize;
            uint32_t itemIndex = virtualIndex % chunkSize;

            Chunk& chunk = m_chunks[chunkIndex];
            T& item = reinterpret_cast<T*>(chunk.data.get())[itemIndex];
            if (chunk.version[itemIndex] == version) {
                return item;
            } else {
                throw std::runtime_error("SlotID is not valid");
            }
        }

    private:
        std::vector<Chunk> m_chunks;
        std::stack<uint32_t> m_free;

        void destroy(SlotID slot) {
            uint32_t virtualIndex = slot.index;
            uint32_t version = slot.version;

            uint32_t chunkIndex = virtualIndex / chunkSize;
            uint32_t itemIndex = virtualIndex % chunkSize;

            Chunk& chunk = m_chunks[chunkIndex];
            T& item = reinterpret_cast<T*>(chunk.data.get())[itemIndex];
            if (chunk.version[itemIndex] == version) {
                chunk.destroy(itemIndex);
                m_free.push(itemIndex);
            } else {
                throw std::runtime_error("SlotID is not valid");
            }
        }
    };
}