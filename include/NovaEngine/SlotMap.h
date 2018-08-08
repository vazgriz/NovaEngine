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

        struct Item {
            T item;
            uint32_t version;
            bool live;

            void destroy() {
                item.~T();
                version++;
                live = false;
            }
        };

        static constexpr size_t chunkSize = std::max<size_t>(4096 / sizeof(T), 1);

    public:
        SlotMap() {}
        SlotMap(const SlotMap& other) = delete;
        SlotMap& operator = (const SlotMap& other) = delete;
        SlotMap(SlotMap&& other) = default;
        SlotMap& operator = (SlotMap&& other) = default;

        ~SlotMap() {
            for (void* ptr : m_items) {
                Item* items = static_cast<Item*>(ptr);
                for (size_t i = 0; i < chunkSize; i++) {
                    if (items[i].live) {
                        items[i].item.~T();
                    }
                }
                free(ptr);
            }
        }

        template<typename... Args>
        SlotHandle<T> allocate(Args... args) {
            uint32_t virtualIndex;

            if (m_free.empty()) {
                Item* newItem = static_cast<Item*>(calloc(chunkSize, sizeof(Item)));
                uint32_t startIndex = static_cast<uint32_t>(m_items.size() * chunkSize);

                for (uint32_t i = 0; i < chunkSize; i++) {
                    m_free.push(startIndex + (chunkSize - i - 1));
                }

                m_items.push_back(newItem);
            }

            virtualIndex = m_free.top();
            m_free.pop();

            uint32_t itemIndex = virtualIndex / chunkSize;
            uint32_t chunkIndex = virtualIndex % chunkSize;

            Item& item = static_cast<Item*>(m_items[itemIndex])[chunkIndex];
            item.live = true;
            new (&item.item) T(std::forward<Args...>(args...));

            return SlotHandle<T>(this, { static_cast<uint32_t>(virtualIndex), item.version });
        }

        T& get(SlotID slot) {
            uint32_t virtualIndex = slot.index;
            uint32_t version = slot.version;

            uint32_t itemIndex = virtualIndex / chunkSize;
            uint32_t chunkIndex = virtualIndex % chunkSize;

            Item& item = static_cast<Item*>(m_items[itemIndex])[chunkIndex];
            if (item.version == version) {
                return item.item;
            } else {
                throw std::runtime_error("SlotID is not valid");
            }
        }

    private:
        std::vector<void*> m_items;
        std::stack<uint32_t> m_free;

        void destroy(SlotID slot) {
            uint32_t virtualIndex = slot.index;
            uint32_t version = slot.version;

            uint32_t itemIndex = virtualIndex / chunkSize;
            uint32_t chunkIndex = virtualIndex % chunkSize;

            Item& item = static_cast<Item*>(m_items[itemIndex])[chunkIndex];
            if (item.version == version) {
                item.destroy();
            } else {
                throw std::runtime_error("SlotID is not valid");
            }
        }
    };
}