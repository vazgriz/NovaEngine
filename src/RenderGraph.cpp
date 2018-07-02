#include "NovaEngine/RenderGraph.h"
#include <queue>
#include "NovaEngine/DirectedAcyclicGraph.h"

using namespace Nova;

RenderNode::RenderNode(const vk::Queue& queue) {
    m_queue = &queue;
    m_family = queue.familyIndex();
}

RenderGraph::RenderGraph(Engine& engine, size_t frames) {
    m_engine = &engine;
}

RenderNode& RenderGraph::addNode(const vk::Queue& queue) {
    m_nodes.emplace_back(std::make_unique<RenderNode>(queue));
    m_nodeSet.insert(m_nodes.back().get());
    return *m_nodes.back();
}

void RenderGraph::addEdge(RenderNode& source, RenderNode& dest) {
    source.m_outNodes.push_back(&dest);
}

void RenderGraph::bake() {
    m_nodeList = topologicalSort<RenderNode>(m_nodeSet, [](const RenderNode* node) {
        return node->m_outNodes;
    });
}