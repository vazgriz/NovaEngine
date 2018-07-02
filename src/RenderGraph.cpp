#include "NovaEngine/RenderGraph.h"
#include <queue>
#include "NovaEngine/DirectedAcyclicGraph.h"

using namespace Nova;

RenderNode::RenderNode(RenderGraph* graph, const vk::Queue& queue) {
    m_graph = graph;
    m_queue = &queue;
    m_family = queue.familyIndex();
}

void RenderNode::preRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) const {
    for (size_t i = 0; i < m_inEvents.size(); i++) {
        if (m_outEvents[i] != nullptr) {
            commandBuffer.waitEvents({ *m_outEvents[i] }, srcStage, dstStage, {}, {}, {});
            commandBuffer.resetEvent(*m_outEvents[i], dstStage);
        } else {
            commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags::None, {}, {}, {});
        }
    }
}

void RenderNode::postRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) const {
    for (size_t i = 0; i < m_inEvents.size(); i++) {
        if (m_inEvents[i] != nullptr) {
            commandBuffer.setEvent(*m_inEvents[i], dstStage);
        } else {
            commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags::None, {}, {}, {});
        }
    }
}

RenderGraph::RenderGraph(Engine& engine, size_t frames) {
    m_engine = &engine;
}

RenderNode& RenderGraph::addNode(const vk::Queue& queue) {
    m_nodes.emplace_back(std::make_unique<RenderNode>(this, queue));
    m_nodeSet.insert(m_nodes.back().get());
    return *m_nodes.back();
}

void RenderGraph::addEdge(RenderNode& source, RenderNode& dest) {
    source.m_outNodes.push_back(&dest);
    dest.m_inNodes.push_back(&source);
}

void RenderGraph::bake() {
    m_nodeList = topologicalSort<RenderNode>(m_nodeSet, [](const RenderNode* node) {
        return node->m_outNodes;
    });

    for (auto node : m_nodeList) {
        for (auto outNode : node->m_outNodes) {
            if (node->m_family == outNode->m_family) {
                vk::Event& event = createEvent(*node->m_queue);
                node->m_outEvents.push_back(&event);
                outNode->m_inEvents.push_back(&event);
            } else {
                node->m_outEvents.push_back(nullptr);
                outNode->m_inEvents.push_back(nullptr);
            }
        }
    }
}

vk::Event& RenderGraph::createEvent(const vk::Queue& queue) {
    vk::EventCreateInfo info = {};

    m_events.emplace_back(std::make_unique<vk::Event>(queue.device(), info));

    return *m_events.back();
}