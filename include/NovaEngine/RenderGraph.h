#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>
#include "NovaEngine/Engine.h"

namespace Nova {
    class RenderGraph;

    class RenderNode {
        friend class RenderGraph;
    public:
        RenderNode(RenderGraph* graph, const vk::Queue& queue);
        RenderNode(const RenderNode& other) = delete;
        RenderNode& operator = (const RenderNode& other) = delete;
        RenderNode(RenderNode&& other) = default;
        RenderNode& operator = (RenderNode&& other) = default;
        ~RenderNode() = default;

        void preRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) const;
        void postRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) const;

    private:
        RenderGraph* m_graph;
        const vk::Queue* m_queue;
        uint32_t m_family;
        std::vector<RenderNode*> m_inNodes;
        std::vector<RenderNode*> m_outNodes;
        std::vector<vk::Event*> m_inEvents;
        std::vector<vk::Event*> m_outEvents;
    };

    class RenderGraph {
        friend class RenderNode;
    public:
        RenderGraph(Engine& engine, size_t frames);
        RenderGraph(const RenderGraph& other) = delete;
        RenderGraph& operator = (const RenderGraph& other) = delete;
        RenderGraph(RenderGraph&& other) = default;
        RenderGraph& operator = (RenderGraph&& other) = default;

        RenderNode& addNode(const vk::Queue& queue);

        void addEdge(RenderNode& source, RenderNode& dest);
        void bake();

    private:
        Engine* m_engine;
        std::vector<std::unique_ptr<RenderNode>> m_nodes;
        std::unordered_set<RenderNode*> m_nodeSet;
        std::vector<RenderNode*> m_nodeList;
        std::vector<std::unique_ptr<vk::Event>> m_events;

        vk::Event& createEvent(const vk::Queue& queue);
    };
}