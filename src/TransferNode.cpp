#include "NovaEngine/TransferNode.h"

using namespace Nova;

TransferNode::TransferNode(Engine& engine, const vk::Queue& queue, FrameGraph& frameGraph, size_t pageSize) : FrameNode(queue, vk::PipelineStageFlags::Transfer, vk::PipelineStageFlags::Transfer) {
    m_engine = &engine;
    m_frameGraph = &frameGraph;
    m_pageSize = pageSize;

    m_bufferUsage = &FrameNode::addBufferUsage(vk::AccessFlags::TransferWrite);
    m_imageUsage = &FrameNode::addImageUsage(vk::AccessFlags::TransferWrite, vk::ImageLayout::TransferDstOptimal);

    findType();

    m_onFrameCountChanged = m_frameGraph->onFrameCountChanged().connectMember<TransferNode>(*this, &TransferNode::resize);
    resize(m_frameGraph->frameCount());
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
    return m_frameGraph->frame() % m_frameGraph->frameCount();
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

std::vector<const vk::CommandBuffer*>& TransferNode::submit(size_t frame, size_t index) {
    m_commandBuffers.clear();
    StagingAllocator& allocator = m_allocators[index];

    vk::CommandBuffer& commandBuffer = commandBuffers()[index];
    commandBuffer.reset({});

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlags::OneTimeSubmit;

    commandBuffer.begin(beginInfo);

    FrameNode::preRecord(commandBuffer);

    for (auto& transfer : m_transfers) {
        if (transfer.buffer != nullptr) {
            commandBuffer.copyBuffer(allocator.buffer(), transfer.buffer->resource(), transfer.bufferCopy);
        } else if (transfer.image != nullptr) {
            vk::ImageSubresourceRange range = {};

            vk::ImageMemoryBarrier barrier = {};
            barrier.image = &transfer.image->resource();
            barrier.oldLayout = vk::ImageLayout::Undefined;
            barrier.newLayout = transfer.imageLayout;
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlags::TransferWrite;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = transfer.bufferImageCopy.imageSubresource.aspectMask;
            barrier.subresourceRange.baseArrayLayer = transfer.bufferImageCopy.imageSubresource.baseArrayLayer;
            barrier.subresourceRange.layerCount = transfer.bufferImageCopy.imageSubresource.layerCount;
            barrier.subresourceRange.baseMipLevel = transfer.bufferImageCopy.imageSubresource.mipLevel;
            barrier.subresourceRange.levelCount = 1;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlags::TopOfPipe, vk::PipelineStageFlags::Transfer, {}, {}, {}, { barrier });
            commandBuffer.copyBufferToImage(allocator.buffer(), transfer.image->resource(), transfer.imageLayout, transfer.bufferImageCopy);
        }
    }

    FrameNode::postRecord(commandBuffer);

    commandBuffer.end();
    m_commandBuffers.push_back(&commandBuffer);

    m_transfers.clear();
    allocator.reset();
    return m_commandBuffers;
}

void TransferNode::transfer(const void* data, const Buffer& buffer, vk::BufferCopy copy) {
    if (buffer.page().mapping() != nullptr) {
        //for buffers that are host-visible (eg UMA)
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

    buffer.registerUsage(m_frameGraph->frame());
    m_bufferUsage->add(*transfer.buffer, copy.dstOffset, copy.size);
}

void TransferNode::transfer(const void* data, const Image& image, vk::ImageLayout imageLayout, vk::BufferImageCopy copy) {
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

    vk::ImageSubresourceRange range = {};
    range.aspectMask = copy.imageSubresource.aspectMask;
    range.baseArrayLayer = copy.imageSubresource.baseArrayLayer;
    range.layerCount = copy.imageSubresource.layerCount;
    range.baseMipLevel = copy.imageSubresource.mipLevel;
    range.levelCount = 1;

    image.registerUsage(m_frameGraph->frame());
    m_imageUsage->add(*transfer.image, range);
}