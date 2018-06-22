#include "NovaEngine/QueueGraph.h"

using namespace Nova;

QueueGraph::QueueGraph(Engine& engine) {
    m_engine = &engine;
    m_renderer = &engine.renderer();
}

void QueueGraph::addNode(const vk::Queue& queue, QueueNode& node) {
    m_nodes[&node] = { &node, &queue };
}

void QueueGraph::addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask) {
    auto& startInfo = m_nodes[&start];
    auto& endInfo = m_nodes[&end];
    startInfo.outNodes.push_back(&endInfo);
    startInfo.info.waitDstStageMask.push_back(waitMask);
    endInfo.inNodes.push_back(&startInfo);
}

void QueueGraph::bake() {
    std::unordered_set<Node*> nodes;

    for (auto& pair : m_nodes) {
        nodes.insert(&pair.second);
    }

    m_nodeList = topologicalSort<Node>(nodes,
        [](const Node* node) {
            return node->outNodes;
        }
    );

    createSemaphores();
    createFences();
}

void QueueGraph::createSemaphores() {
    for (auto node : m_nodeList) {
        for (auto outNode : node->outNodes) {
            vk::SemaphoreCreateInfo info = {};
            
            std::unique_ptr<vk::Semaphore> semaphore = std::make_unique<vk::Semaphore>(m_renderer->device(), info);

            m_semaphores.emplace_back(std::move(semaphore));
            node->info.signalSemaphores.push_back(*m_semaphores.back());
            outNode->info.waitSemaphores.push_back(*m_semaphores.back());
        }
    }
}

void QueueGraph::createFences() {
    for (auto node : m_nodeList) {
        vk::FenceCreateInfo info = {};
        info.flags = vk::FenceCreateFlags::Signaled;

        m_fences.emplace_back(std::make_unique<vk::Fence>(m_renderer->device(), info));
        node->fence = m_fences.back().get();
    }
}

void QueueGraph::submit() {
    for (auto node : m_nodeList) {
        node->node->preSubmit();
    }

    for (auto node : m_nodeList) {
        node->info.commandBuffers = node->node->getCommands();
        node->queue->submit({node->info }, node->fence);
    }

    for (auto node : m_nodeList) {
        node->node->postSubmit();
    }
}