#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/QueueGraph.h>
#include <NovaEngine/IResourceAllocator.h>
#include <NovaEngine/StagingAllocator.h>

namespace Nova {
    class TransferNode : public QueueNode {
        struct Transfer {
            const Buffer* buffer;
            const Image* image;
            vk::BufferCopy bufferCopy;
            vk::BufferImageCopy bufferImageCopy;
            vk::ImageLayout imageLayout;
        };

    public:
        TransferNode(Engine& engine, const vk::Queue& queue, QueueGraph& queueGraph, size_t pageSize);
        TransferNode(const TransferNode& other) = delete;
        TransferNode& operator = (const TransferNode& other) = delete;
        TransferNode(TransferNode&& other) = default;
        TransferNode& operator = (TransferNode&& other) = default;

        const std::vector<const vk::CommandBuffer*>& getCommands(size_t index) override;

        void transfer(void* data, const Buffer& buffer, vk::BufferCopy copy);
        void transfer(void* data, const Image& image, vk::ImageLayout imageLayout, vk::BufferImageCopy copy);

    private:
        Engine* m_engine;
        QueueGraph* m_queueGraph;
        Slot<size_t> m_onFrameCountChanged;
        std::vector<const vk::CommandBuffer*> m_commandBuffers;
        std::vector<StagingAllocator> m_allocators;
        size_t m_pageSize;
        uint32_t m_type;
        std::vector<Transfer> m_transfers;

        void findType();
        size_t getFrame();
        void resize(size_t frames);
    };
}