#include "NovaEngine/QueueGraph.h"

using namespace Nova;

QueueNode::QueueNode(const vk::Queue& queue) {
    m_queue = &queue;
}

QueueGraph::QueueGraph(Engine& engine, size_t frames) {
    m_engine = &engine;
    m_renderer = &engine.renderer();

    setFrames(frames);
}

QueueGraph::~QueueGraph() {
    for (auto& fences : m_fences) {
        vk::Fence::wait(m_engine->renderer().device(), fences, true);
    }
}

void QueueGraph::setFrames(size_t frames) {
    if (m_frameCount == frames) return;
    internalSetFrames(frames);
}

void QueueGraph::internalSetFrames(size_t frames) {
    m_frameCount = frames;

    for (auto& v : m_fences) {
        vk::Fence::wait(m_engine->renderer().device(), v, true);
    }

    m_fences.clear();

    for (size_t i = 0; i < frames; i++) {
        m_fences.emplace_back();
        auto& nodeFences = m_fences.back();

        for (auto& node : m_nodeList) {
            vk::FenceCreateInfo info = {};
            info.flags = vk::FenceCreateFlags::Signaled;

            nodeFences.emplace_back(m_engine->renderer().device(), info);
        }
    }
}

void QueueGraph::addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask) {
    auto& startInfo = m_nodes[&start];
    auto& endInfo = m_nodes[&end];
    startInfo.outNodes.push_back(&endInfo);
    startInfo.info.waitDstStageMask.push_back(waitMask);
    endInfo.inNodes.push_back(&startInfo);
}

void QueueGraph::addExternalWait(QueueNode& node, vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask) {
    auto& info = m_nodes[&node];
    info.externalWaits.push_back(&semaphore);
    info.externalStageMasks.push_back(stageMask);
}

void QueueGraph::addExternalSignal(QueueNode& node, vk::Semaphore& semaphore) {
    auto& info = m_nodes[&node];
    info.externalSignals.push_back(&semaphore);
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
    internalSetFrames(m_fences.size());
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

        for (size_t i = 0; i < node->externalWaits.size(); i++) {
            node->info.waitSemaphores.push_back(*node->externalWaits[i]);
            node->info.waitDstStageMask.push_back(node->externalStageMasks[i]);
        }

        for (auto semaphore : node->externalSignals) {
            node->info.signalSemaphores.push_back(*semaphore);
        }
    }
}

void QueueGraph::submit() {
    size_t index = m_frame % m_fences.size();

    for (auto node : m_nodeList) {
        node->node->preSubmit();
    }

    for (size_t i = 0; i < m_nodeList.size(); i++) {
        auto node = m_nodeList[i];
        node->info.commandBuffers.clear();

        m_fences[index][i].wait();
        m_fences[index][i].reset();

        auto commandBuffers = node->node->getCommands();

        for (auto commandBuffer : commandBuffers) {
            node->info.commandBuffers.push_back(*commandBuffer);
        }

        node->queue->submit({ node->info }, &m_fences[index][i]);
    }

    for (auto node : m_nodeList) {
        node->node->postSubmit();
    }

    m_frame++;
}

size_t QueueGraph::completedFrames() {
    if (m_frameCount > m_frame) return 0;
    return m_frame - m_frameCount;
}