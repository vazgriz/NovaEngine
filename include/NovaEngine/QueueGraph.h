#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>
#include "NovaEngine/Signal.h"

namespace Nova {
    class Engine;
    class Renderer;
    class QueueGraph;

    class QueueNode {
        friend class QueueGraph;
    public:
        QueueNode(const vk::Queue& queue);
        QueueNode(const QueueNode& other) = delete;
        QueueNode& operator = (const QueueNode& other) = delete;
        QueueNode(QueueNode&& other) = default;
        QueueNode& operator = (QueueNode&& other) = default;
        virtual ~QueueNode() = default;

        virtual void preSubmit(size_t frame) {}
        virtual const std::vector<const vk::CommandBuffer*>& getCommands(size_t frame, size_t index) = 0;
        virtual void postSubmit(size_t frame) {}

        const vk::Queue& queue() const { return *m_queue; }

    protected:
        vk::CommandPool& commandPool() { return *m_commandPool; }
        std::vector<vk::CommandBuffer>& commandBuffers() { return m_commandBuffers; }

    private:
        const vk::Queue* m_queue;
        std::unique_ptr<vk::CommandPool> m_commandPool;
        std::vector<vk::CommandBuffer> m_commandBuffers;

        void createCommandPool();
        void createCommandBuffers(size_t count);
    };

    class QueueGraph {
    public:
        struct Node {
            QueueNode* node;
            const vk::Queue* queue;
            std::vector<Node*> inNodes;
            std::vector<Node*> outNodes;
            std::vector<vk::Semaphore*> externalWaits;
            std::vector<vk::PipelineStageFlags> externalStageMasks;
            std::vector<vk::Semaphore*> externalSignals;
            vk::SubmitInfo info;
        };

        QueueGraph(Engine& engine);
        QueueGraph(const QueueGraph& other) = delete;
        QueueGraph& operator = (const QueueGraph& other) = delete;
        QueueGraph(QueueGraph&& other) = default;
        QueueGraph& operator = (QueueGraph&& other) = default;

        void addNode(QueueNode& node);

        void addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask);
        void addExternalWait(QueueNode& node, vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask);
        void addExternalSignal(QueueNode& node, vk::Semaphore& semaphore);

        void setFrames(size_t frames);
        Signal<size_t>& onFrameCountChanged() { return *m_onFrameCountChanged; }
        void bake();

        void submit();
        size_t frame() const { return m_frame; }
        size_t completedFrames() const;
        size_t frameCount() const { return m_frameCount; }
        void wait();

    private:
        Engine* m_engine;
        Renderer* m_renderer;
        std::unordered_map<QueueNode*, Node> m_nodes;
        std::vector<Node*> m_nodeList;
        std::vector<std::unique_ptr<vk::Semaphore>> m_semaphores;
        std::vector<std::vector<vk::Fence>> m_fences;
        size_t m_frameCount = 0;
        size_t m_frame = 1;
        std::unique_ptr<Signal<size_t>> m_onFrameCountChanged;

        void createSemaphores();
        void internalSetFrames(size_t frames);
    };
}