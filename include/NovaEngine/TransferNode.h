#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/FrameGraph.h"
#include "NovaEngine/IResourceAllocator.h"
#include "NovaEngine/StagingAllocator.h"
#include <boost/signals2.hpp>

namespace Nova {
    class TransferNode : public FrameNode {
        struct Transfer {
            const Buffer* buffer;
            const Image* image;
            vk::BufferCopy bufferCopy;
            vk::BufferImageCopy bufferImageCopy;
            vk::ImageLayout imageLayout;
        };

    public:
        TransferNode(Engine& engine, const vk::Queue& queue, FrameGraph& frameGraph, size_t pageSize);
        TransferNode(const TransferNode& other) = delete;
        TransferNode& operator = (const TransferNode& other) = delete;
        TransferNode(TransferNode&& other) = default;
        TransferNode& operator = (TransferNode&& other) = default;

        std::vector<const vk::CommandBuffer*>& submit(size_t frame, size_t index) override;

        void transfer(const void* data, const Buffer& buffer, vk::BufferCopy copy);
        void transfer(const void* data, const Image& image, vk::ImageLayout imageLayout, vk::BufferImageCopy copy);

    private:
        Engine* m_engine;
        FrameGraph* m_frameGraph;
        BufferUsage* m_bufferUsage;
        ImageUsage* m_imageUsage;
        boost::signals2::scoped_connection m_onFrameCountChanged;
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