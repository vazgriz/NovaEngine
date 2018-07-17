#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>
#include "NovaEngine/DirectedAcyclicGraph.h"
#include "NovaEngine/Engine.h"

namespace Nova {
    class QueueGraph;

    class QueueNode {
        friend class QueueGraph;
    public:
        QueueNode(const vk::Queue& queue);
        virtual ~QueueNode() = default;

        virtual void preSubmit() {}
        virtual const std::vector<const vk::CommandBuffer*>& getCommands() = 0;
        virtual void postSubmit() {}

        const vk::Queue& queue() const { return *m_queue; }

    private:
        const vk::Queue* m_queue;
    };

    class QueueGraph {
    public:
        struct Node {
            std::unique_ptr<QueueNode> node;
            const vk::Queue* queue;
            std::vector<Node*> inNodes;
            std::vector<Node*> outNodes;
            std::vector<vk::Semaphore*> externalWaits;
            std::vector<vk::PipelineStageFlags> externalStageMasks;
            std::vector<vk::Semaphore*> externalSignals;
            vk::SubmitInfo info;
        };

        QueueGraph(Engine& engine, size_t frames);
        QueueGraph(const QueueGraph& other) = delete;
        QueueGraph& operator = (const QueueGraph& other) = delete;
        QueueGraph(QueueGraph&& other) = default;
        QueueGraph& operator = (QueueGraph&& other) = default;
        ~QueueGraph();

        template<typename T, class... Types>
        T& addNode(Types&&... args) {
            std::unique_ptr<T> temp = std::make_unique<T>(std::forward<Types>(args)...);
            T* ptr = temp.get();
            m_nodes[ptr] = { std::move(temp) };
            Node& node = m_nodes[ptr];
            node.queue = node.node->m_queue;
            return *static_cast<T*>(node.node.get());
        }

        void addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask);
        void addExternalWait(QueueNode& node, vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask);
        void addExternalSignal(QueueNode& node, vk::Semaphore& semaphore);

        void setFrames(size_t frames);
        void bake();

        void submit();
        size_t frame() const { return m_frame; }
        size_t completedFrames() const;
        size_t frameCount() const { return m_frameCount; }

    private:
        Engine* m_engine;
        Renderer* m_renderer;
        std::unordered_map<QueueNode*, Node> m_nodes;
        std::vector<Node*> m_nodeList;
        std::vector<std::unique_ptr<vk::Semaphore>> m_semaphores;
        std::vector<std::vector<vk::Fence>> m_fences;
        size_t m_frameCount = 0;
        size_t m_frame = 1;

        void createSemaphores();
        void internalSetFrames(size_t frames);
    };
}