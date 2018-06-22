#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>
#include "NovaEngine/DirectedAcyclicGraph.h"
#include "NovaEngine/Engine.h"

namespace Nova {
    class QueueNode {
    public:
        virtual void preSubmit() {}
        virtual const std::vector<std::reference_wrapper<const vk::CommandBuffer>>& getCommands() = 0;
        virtual void postSubmit() {}
    };

    class QueueGraph {
    public:
        struct Node {
            QueueNode* node;
            const vk::Queue* queue;
            const vk::Fence* fence;
            std::vector<Node*> inNodes;
            std::vector<Node*> outNodes;
            vk::SubmitInfo info;
        };

        QueueGraph(Engine& engine);
        QueueGraph(const QueueGraph& other) = delete;
        QueueGraph& operator = (const QueueGraph& other) = delete;
        QueueGraph(QueueGraph&& other) = default;
        QueueGraph& operator = (QueueGraph&& other) = default;

        void addNode(const vk::Queue& queue, QueueNode& node);
        void addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask);
        void bake();

        void submit();

    private:
        Engine* m_engine;
        Renderer* m_renderer;
        std::unordered_map<QueueNode*, Node> m_nodes;
        std::vector<Node*> m_nodeList;
        std::vector<std::unique_ptr<vk::Semaphore>> m_semaphores;
        std::vector<std::unique_ptr<vk::Fence>> m_fences;

        void createSemaphores();
        void createFences();
    };
}