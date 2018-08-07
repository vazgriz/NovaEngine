#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/Allocator.h>

namespace Nova {
    class Engine;
    class TransferNode;

    class VertexData {
    public:
        VertexData(BufferAllocator& allocator);
        VertexData(const VertexData& other) = delete;
        VertexData& operator = (const VertexData& other) = delete;
        VertexData(VertexData&& other) = default;
        VertexData& operator = (VertexData&& other) = default;

        vk::Format format() const { return m_format; }
        size_t vertexCount() const { return m_vertexCount; }
        size_t size() const { return m_size; }
        Buffer& buffer() const { return *m_buffer; }

        void fill(TransferNode& transferNode, vk::Format format, const void* data, size_t vertexCount);

    private:
        BufferAllocator* m_allocator;
        vk::Format m_format;
        size_t m_vertexCount;
        size_t m_size;
        std::unique_ptr<Buffer> m_buffer;

        void createBuffer();
    };

    class IndexData {
    public:
        IndexData(BufferAllocator& allocator);
        IndexData(const IndexData& other) = delete;
        IndexData& operator = (const IndexData& other) = delete;
        IndexData(IndexData&& other) = default;
        IndexData& operator = (IndexData&& other) = default;

        vk::IndexType type() const { return m_type; }
        size_t indexCount() const { return m_indexCount; }
        size_t size() const { return m_size; }
        Buffer& buffer() const { return *m_buffer; }

        void fill(TransferNode& transferNode, const std::vector<uint32_t>& indices);
        void fill(TransferNode& transferNode, const std::vector<uint16_t>& indices);

    private:
        BufferAllocator* m_allocator;
        vk::IndexType m_type;
        size_t m_indexCount;
        size_t m_size;
        std::unique_ptr<Buffer> m_buffer;

        void createBuffer();
    };

    class Mesh {
    public:
        Mesh(uint32_t firstBinding = 0);
        Mesh(const Mesh& other) = delete;
        Mesh& operator = (const Mesh& other) = delete;
        Mesh(Mesh&& other) = default;
        Mesh& operator = (Mesh&& other) = default;

        void addVertexData(std::shared_ptr<VertexData> data, size_t offset = 0);
        void setVertexOffset(size_t index, size_t offset);
        VertexData& getVertexData(size_t index) { return *m_vertexData[index]; }

        void setIndexData(std::shared_ptr<IndexData> data, size_t offset = 0);
        void setIndexOffset(size_t offset);
        IndexData* getIndexData() { return m_indexData.get(); }

        void bind(vk::CommandBuffer& commandBuffer);
        void registerUsage(size_t frame);

        std::vector<vk::VertexInputAttributeDescription> getAttributes();
        std::vector<vk::VertexInputBindingDescription> getBindings();

    private:
        uint32_t m_firstBinding;
        std::vector<std::shared_ptr<VertexData>> m_vertexData;
        std::vector<size_t> m_offsets;
        std::shared_ptr<IndexData> m_indexData;
        size_t m_indexOffset;
    };
}