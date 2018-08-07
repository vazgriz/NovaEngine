#include "NovaEngine/Mesh.h"
#include "NovaEngine/TransferNode.h"

using namespace Nova;

VertexData::VertexData(BufferAllocator& allocator, vk::Format format) {
    m_allocator = &allocator;
    m_format = format;
}

void VertexData::fill(TransferNode& transferNode, const void* data, size_t vertexCount) {
    m_vertexCount = vertexCount;
    createBuffer();

    vk::BufferCopy copy = {};
    copy.size = m_size;
    transferNode.transfer(data, *m_buffer, copy);
}

void VertexData::createBuffer() {
    m_size = m_vertexCount * vk::getFormatSize(m_format);

    vk::BufferCreateInfo info = {};
    info.size = m_size;
    info.usage = vk::BufferUsageFlags::VertexBuffer | vk::BufferUsageFlags::TransferDst;

    if (m_buffer != nullptr) {
        size_t existingSize = m_buffer->size();

        vk::Buffer temp = vk::Buffer(m_allocator->engine().renderer().device(), info);
        size_t tempSize = temp.requirements().size;

        if (tempSize <= existingSize && tempSize >= (existingSize / 2)) {
            return;
        }
    }

    m_buffer = std::make_unique<Buffer>(m_allocator->allocate(info, vk::MemoryPropertyFlags::DeviceLocal, {}));
}

IndexData::IndexData(BufferAllocator& allocator) {
    m_allocator = &allocator;
}

void IndexData::fill(TransferNode& transferNode, const std::vector<uint32_t>& indices) {
    m_type = vk::IndexType::Uint32;
    m_indexCount = indices.size();
    m_size = indices.size() * sizeof(uint32_t);

    createBuffer();

    vk::BufferCopy copy = {};
    copy.size = m_size;

    transferNode.transfer(indices.data(), *m_buffer, copy);
}

void IndexData::fill(TransferNode& transferNode, const std::vector<uint16_t>& indices) {
    m_type = vk::IndexType::Uint16;
    m_indexCount = indices.size();
    m_size = indices.size() * sizeof(uint16_t);

    createBuffer();

    vk::BufferCopy copy = {};
    copy.size = m_size;

    transferNode.transfer(indices.data(), *m_buffer, copy);
}

void IndexData::createBuffer() {
    if (m_buffer != nullptr) {
        size_t existingSize = m_buffer->size();
        if (m_size <= existingSize && m_size >= (existingSize / 2)) {
            return;
        }
    }

    vk::BufferCreateInfo info = {};
    info.size = m_size;
    info.usage = vk::BufferUsageFlags::IndexBuffer | vk::BufferUsageFlags::TransferDst;

    m_buffer = std::make_unique<Buffer>(m_allocator->allocate(info, vk::MemoryPropertyFlags::DeviceLocal, {}));
}

Mesh::Mesh(uint32_t firstBinding) {
    m_firstBinding = firstBinding;
}

void Mesh::addVertexData(std::shared_ptr<VertexData> data, size_t offset) {
    m_vertexData.push_back(data);
    m_offsets.push_back(offset);
}

void Mesh::setVertexOffset(size_t index, size_t offset) {
    m_offsets[index] = offset;
}

void Mesh::setIndexData(std::shared_ptr<IndexData> data, size_t offset) {
    m_indexData = data;
    m_indexOffset = offset;
}

void Mesh::setIndexOffset(size_t offset) {
    m_indexOffset = offset;
}

void Mesh::bind(vk::CommandBuffer& commandBuffer) {
    std::vector<std::reference_wrapper<const vk::Buffer>> buffers;

    for (auto& vertexData : m_vertexData) {
        buffers.push_back(vertexData->buffer().resource());
    }

    commandBuffer.bindVertexBuffers(m_firstBinding, buffers, m_offsets);

    if (m_indexData != nullptr) {
        commandBuffer.bindIndexBuffer(m_indexData->buffer().resource(), m_indexOffset, m_indexData->type());
    }
}

void Mesh::registerUsage(size_t frame) {
    for (auto& vertexData : m_vertexData) {
        vertexData->buffer().registerUsage(frame);
    }

    if (m_indexData != nullptr) {
        m_indexData->buffer().registerUsage(frame);
    }
}

std::vector<vk::VertexInputAttributeDescription> Mesh::getAttributes() {
    std::vector<vk::VertexInputAttributeDescription> attributes;

    for (size_t i = 0; i < m_vertexData.size(); i++) {
        auto& vertexData = m_vertexData[i];

        vk::VertexInputAttributeDescription attribute = {};
        attribute.binding = static_cast<uint32_t>(i);
        attribute.location = static_cast<uint32_t>(i);
        attribute.format = vertexData->format();
        attribute.offset = 0;

        attributes.push_back(attribute);
    }

    return attributes;
}

std::vector<vk::VertexInputBindingDescription> Mesh::getBindings() {
    std::vector<vk::VertexInputBindingDescription> bindings = {};

    for (size_t i = 0; i < m_vertexData.size(); i++) {
        auto& vertexData = m_vertexData[i];

        vk::VertexInputBindingDescription binding = {};
        binding.binding = static_cast<uint32_t>(i);
        binding.stride = static_cast<uint32_t>(vk::getFormatSize(vertexData->format()));
        binding.inputRate = vk::VertexInputRate::Vertex;

        bindings.push_back(binding);
    }

    return bindings;
}