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
        virtual ~RenderNode() = default;

        virtual void preRender() {}
        virtual std::vector<const vk::CommandBuffer*> render() = 0;
        virtual void postRender() {}

        void addExternalWait(vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask);
        void addExternalSignal(vk::Semaphore& semaphore);

    protected:
        const vk::Queue* m_queue;
        uint32_t m_family;

    private:
        std::vector<RenderNode*> m_outNodes;
        std::vector<vk::Semaphore*> m_externalWaits;
        std::vector<vk::PipelineStageFlags> m_externalStageMasks;
        std::vector<vk::Semaphore*> m_externalSignals;
    };

    class RenderGraph {
        struct RenderGroup {
            uint32_t family;
            std::vector<RenderNode*> nodes;
            vk::SubmitInfo info;
            const vk::Queue* queue;
        };

    public:
        RenderGraph(Engine& engine, size_t frames);
        RenderGraph(const RenderGraph& other) = delete;
        RenderGraph& operator = (const RenderGraph& other) = delete;
        RenderGraph(RenderGraph&& other) = default;
        RenderGraph& operator = (RenderGraph&& other) = default;
        ~RenderGraph();

        template <class T>
        T& addNode(T&& node) {
            static_assert(std::is_base_of<RenderNode, T>::value, "T must inherit from RenderNode");
            m_nodes.emplace_back(std::make_unique<T>(std::move(node)));
            m_nodeSet.insert(m_nodes.back().get());
            return *static_cast<T*>(m_nodes.back().get());
        }

        void setFrames(size_t frames);
        void addEdge(RenderNode& source, RenderNode& dest);
        void bake();
        void submit();

    private:
        Engine* m_engine;
        std::vector<std::unique_ptr<RenderNode>> m_nodes;
        std::unordered_set<RenderNode*> m_nodeSet;
        std::vector<RenderNode*> m_nodeList;
        std::vector<RenderGroup> m_groups;
        std::vector<std::vector<vk::Fence>> m_fences;
        size_t m_frame = 0;

        void createGroups();
    };
}