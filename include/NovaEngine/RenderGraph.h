#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>
#include "NovaEngine/Engine.h"

namespace Nova {
    class RenderNode {
        friend class RenderGraph;
    public:
        RenderNode(const vk::Queue& queue);
        RenderNode(const RenderNode& other) = delete;
        RenderNode& operator = (const RenderNode& other) = delete;
        RenderNode(RenderNode&& other) = default;
        RenderNode& operator = (RenderNode&& other) = default;
        ~RenderNode() = default;

    protected:
        const vk::Queue* m_queue;
        uint32_t m_family;

    private:
        std::vector<RenderNode*> m_outNodes;
    };

    class RenderGraph {
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
    };
}