#include "NovaEngine/TransferNode.h"

using namespace Nova;

TransferNode::TransferNode(Engine& engine, const vk::Queue& queue, QueueGraph& queueGraph, size_t pageSize) : QueueNode(queue) {
    m_engine = &engine;
    m_queueGraph = &queueGraph;
    m_pageSize = pageSize;

    findType();

    m_onFrameCountChanged = m_queueGraph->onFrameCountChanged().connectMember<TransferNode>(*this, &TransferNode::resize);
    resize(m_queueGraph->frameCount());
}

void TransferNode::findType() {
    vk::BufferCreateInfo info = {};
    info.size = m_pageSize;
    info.usage = vk::BufferUsageFlags::TransferSrc;

    vk::Buffer buffer = vk::Buffer(m_engine->renderer().device(), info);
    vk::MemoryRequirements requirements = buffer.requirements();

    vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlags::HostVisible | vk::MemoryPropertyFlags::HostCoherent;
    auto& properties = m_engine->memory().properties();

    for (uint32_t i = 0; i < properties.memoryTypes.size(); i++) {
        if ((requirements.memoryTypeBits & (1 << i)) != 0) {
            auto& type = properties.memoryTypes[i];
            if ((type.propertyFlags & flags) == flags) {
                m_type = i;
                return;
            }
        }
    }
}

size_t TransferNode::getFrame() {
    return m_queueGraph->frame() % m_queueGraph->frameCount();
}

void TransferNode::resize(size_t frames) {
    if (frames == m_allocators.size()) {
        return;
    } else if (frames < m_allocators.size()) {
        m_allocators.erase(m_allocators.begin() + frames);
    } else {
        for (size_t i = m_allocators.size(); i < frames; i++) {
            m_allocators.emplace_back(*m_engine, m_pageSize, m_type);
        }
    }
}

const std::vector<const vk::CommandBuffer*>& TransferNode::getCommands(size_t index) {
    m_commandBuffers.clear();
    StagingAllocator& allocator = m_allocators[index];

    vk::CommandBuffer& commandBuffer = commandBuffers()[index];
    commandBuffer.reset({});

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlags::OneTimeSubmit;

    commandBuffer.begin(beginInfo);

    for (auto& transfer : m_transfers) {
        if (transfer.buffer != nullptr) {
            commandBuffer.copyBuffer(allocator.buffer(), transfer.buffer->resource(), transfer.bufferCopy);
        } else if (transfer.image != nullptr) {
            commandBuffer.copyBufferToImage(allocator.buffer(), transfer.image->resource(), transfer.imageLayout, transfer.bufferImageCopy);
        }
    }

    commandBuffer.end();
    m_commandBuffers.push_back(&commandBuffer);

    m_transfers.clear();
    allocator.reset();
    return m_commandBuffers;
}

void TransferNode::transfer(void* data, const Buffer& buffer, vk::BufferCopy copy) {
    if (buffer.page().mapping() != nullptr) {
        //for buffers that are host-visible (eg NUMA)
        void* dest = static_cast<void*>(static_cast<char*>(buffer.page().mapping()) + buffer.offset() + copy.dstOffset);
        memcpy(dest, data, copy.size);
        return;
    }

    size_t index = getFrame();
    StagingAllocator& allocator = m_allocators[index];
    size_t offset = allocator.stage(data, copy.size);

    Transfer transfer = {};
    transfer.buffer = &buffer;
    transfer.bufferCopy = { offset, copy.dstOffset, copy.size };
    m_transfers.push_back(transfer);
}

void TransferNode::transfer(void* data, const Image& image, vk::ImageLayout imageLayout, vk::BufferImageCopy copy) {
    //can't copy directly into images, so it must go through staging
    size_t index = getFrame();
    StagingAllocator& allocator = m_allocators[index];

    size_t texelsToCopy = copy.imageExtent.width * copy.imageExtent.height * copy.imageExtent.depth;
    size_t size = texelsToCopy * vk::getFormatSize(image.resource().format());

    size_t offset = allocator.stage(data, size);

    Transfer transfer = {};
    transfer.image = &image;
    transfer.bufferImageCopy = { offset, 0, 0, copy.imageSubresource, copy.imageOffset, copy.imageExtent };
    transfer.imageLayout = imageLayout;
    m_transfers.push_back(transfer);
}