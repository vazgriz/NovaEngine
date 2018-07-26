#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <type_traits>
#include <unordered_set>
#include "NovaEngine/IResourceAllocator.h"
#include "NovaEngine/Signal.h"

namespace Nova {
    class Engine;
    class FrameNode;
    class BufferUsage;
    class ImageUsage;

    class FrameGraph {
        friend class FrameNode;
        friend class BufferUsage;
        friend class ImageUsage;

        struct Group {
            Group(const vk::Queue& queue);
            std::vector<FrameNode*> nodes;
            const vk::Queue* queue;
            uint32_t family;
            vk::PipelineStageFlags sourceStages;
            vk::PipelineStageFlags destStages;
            vk::SubmitInfo info;

            void addNode(FrameNode* node);
            void submit(size_t frame, size_t index, vk::Fence& fence);
        };

        struct Event {
            Event(FrameNode& source, FrameNode& dest);
            void buildBarriers();
            void recordSource(vk::CommandBuffer& commandBuffer);
            void recordDest(vk::CommandBuffer& commandBuffer);

            FrameNode* source;
            FrameNode* dest;
            std::unique_ptr<vk::Event> event;
            std::vector<vk::BufferMemoryBarrier> sourceBufferBarriers;
            std::vector<vk::ImageMemoryBarrier> sourceImageBarriers;
            std::vector<vk::BufferMemoryBarrier> destBufferBarriers;
            std::vector<vk::ImageMemoryBarrier> destImageBarriers;
        };

    public:
        FrameGraph(Engine& engine);
        FrameGraph(const FrameGraph& other) = delete;
        FrameGraph& operator = (const FrameGraph& other) = delete;
        FrameGraph(FrameGraph&& other) = default;
        FrameGraph& operator = (FrameGraph&& other) = default;

        size_t frame() const { return m_frame; }
        size_t frameCount() const { return m_frameCount; }
        Signal<size_t>& onFrameCountChanged() const { return *m_onFrameCountChanged; }

        void addNode(FrameNode& node);
        void addEdge(FrameNode& source, FrameNode& dest);
        void setFrameCount(size_t frames);
        void bake();
        void submit();
        size_t completedFrames() const;

    private:
        Engine* m_engine;
        std::vector<FrameNode*> m_nodes;
        std::vector<FrameNode*> m_nodeList;
        size_t m_frameCount;
        size_t m_frame = 1;
        std::vector<std::unique_ptr<Group>> m_groups;
        std::vector<std::unique_ptr<vk::Semaphore>> m_semaphores;
        std::vector<std::unique_ptr<Event>> m_events;
        std::vector<std::vector<vk::Fence>> m_fences;
        std::unique_ptr<Signal<size_t>> m_onFrameCountChanged;

        void internalSetFrames(size_t frames);
        void createGroups();
        void createSemaphores();
    };

    class BufferUsage {
        friend class FrameNode;
        friend struct FrameGraph::Event;

        struct Instance {
            vk::Buffer* buffer;
            size_t offset;
            size_t size;
            BufferUsage* usage;
        };

    public:
        BufferUsage(FrameNode* node, vk::AccessFlags accessMask);

        void add(const Buffer& buffer, size_t offset, size_t size);

    private:
        FrameNode* m_node;
        vk::AccessFlags m_accessMask;
    };

    class ImageUsage {
        friend class FrameNode;
        friend struct FrameGraph::Event;

        struct Instance {
            vk::Image* image;
            vk::ImageSubresourceRange range;
            ImageUsage* usage;
        };

    public:
        ImageUsage(FrameNode* node, vk::AccessFlags accessMask, vk::ImageLayout layout);

        void add(const Image& image, vk::ImageSubresourceRange range);

    private:
        FrameNode* m_node;
        vk::AccessFlags m_accessMask;
        vk::ImageLayout m_layout;
    };

    class FrameNode {
        friend class FrameGraph;
        friend class BufferUsage;
        friend class ImageUsage;

    public:
        FrameNode(const vk::Queue& queue, vk::PipelineStageFlags sourceStages, vk::PipelineStageFlags destStages);
        FrameNode(const FrameNode& other) = delete;
        FrameNode& operator = (const FrameNode& other) = delete;
        FrameNode(FrameNode&& other) = default;
        FrameNode& operator = (FrameNode&& other) = default;
        virtual ~FrameNode() { }

        const vk::Queue& queue() const { return *m_queue; }
        void addExternalWait(vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask);
        void addExternalSignal(vk::Semaphore& semaphore);

        void preRecord(vk::CommandBuffer& commandBuffer);
        void postRecord(vk::CommandBuffer& commandBuffer);

        virtual void preSubmit(size_t frame) {};
        virtual std::vector<const vk::CommandBuffer*>& submit(size_t frame, size_t index) = 0;
        virtual void postSubmit(size_t frame) {};

    protected:
        vk::CommandPool& commandPool() const { return *m_pool; }
        std::vector<vk::CommandBuffer>& commandBuffers() { return m_commandBuffers; }
        BufferUsage& addBufferUsage(vk::AccessFlags accessMask);
        ImageUsage& addImageUsage(vk::AccessFlags accessMask, vk::ImageLayout layout);

    private:
        FrameGraph* m_graph;
        FrameGraph::Group* m_group;
        const vk::Queue* m_queue;
        uint32_t m_family;
        vk::PipelineStageFlags m_sourceStages;
        vk::PipelineStageFlags m_destStages;
        std::unique_ptr<vk::CommandPool> m_pool;
        std::vector<vk::CommandBuffer> m_commandBuffers;
        std::vector<vk::Semaphore*> m_externalSignals;
        std::vector<vk::Semaphore*> m_externalWaits;
        std::vector<vk::PipelineStageFlags> m_externalWaitMasks;
        std::vector<FrameGraph::Event*> m_inEvents;
        std::vector<FrameGraph::Event*> m_outEvents;
        std::vector<std::unique_ptr<BufferUsage>> m_bufferUsages;
        std::vector<std::unique_ptr<ImageUsage>> m_imageUsages;
        std::unordered_map<vk::Buffer*, BufferUsage::Instance> m_bufferMap;
        std::unordered_map<vk::Image*, ImageUsage::Instance> m_imageMap;

        void createCommandPool();
        void createCommandBuffers(size_t frames);
        bool addBuffer(vk::Buffer& buffer, BufferUsage::Instance usage);
        bool addImage(vk::Image& image, ImageUsage::Instance usage);
        void clearInstances();
    };
}