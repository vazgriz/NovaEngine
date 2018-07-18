#include "NovaEngine/QueueGraph.h"

using namespace Nova;

QueueNode::QueueNode(const vk::Queue& queue) {
    m_queue = &queue;

    createCommandPool();
}

void QueueNode::createCommandPool() {
    vk::CommandPoolCreateInfo info = {};
    info.queueFamilyIndex = m_queue->familyIndex();
    info.flags = vk::CommandPoolCreateFlags::ResetCommandBuffer;

    m_commandPool = std::make_unique<vk::CommandPool>(m_queue->device(), info);
}

void QueueNode::createCommandBuffers(size_t count) {
    vk::CommandBufferAllocateInfo info = {};
    info.commandBufferCount = static_cast<uint32_t>(count);
    info.commandPool = m_commandPool.get();

    m_commandBuffers = m_commandPool->allocate(info);
}

QueueGraph::QueueGraph(Engine& engine, size_t frames) {
    m_engine = &engine;
    m_renderer = &engine.renderer();
    m_onFrameCountChanged = std::make_unique<Signal<size_t>>();

    setFrames(frames);
}

void QueueGraph::wait() {
    for (auto& fences : m_fences) {
        vk::Fence::wait(m_engine->renderer().device(), fences, true);
    }
}

void QueueGraph::addNode(QueueNode& node) {
    m_nodes[&node] = { &node };
    Node& internalNode = m_nodes[&node];
    internalNode.queue = internalNode.node->m_queue;
}

void QueueGraph::setFrames(size_t frames) {
    if (m_frameCount == frames) return;
    internalSetFrames(frames);
}

void QueueGraph::internalSetFrames(size_t frames) {
    m_frameCount = frames;

    for (auto& v : m_fences) {
        if (v.size() > 0) {
            vk::Fence::wait(m_engine->renderer().device(), v, true);
        }
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

    for (auto& node : m_nodeList) {
        node->node->createCommandBuffers(m_frameCount);
    }

    m_onFrameCountChanged->emit(m_frameCount);
}

void QueueGraph::addEdge(QueueNode& start, QueueNode& end, vk::PipelineStageFlags waitMask) {
    auto& startInfo = m_nodes[&start];
    auto& endInfo = m_nodes[&end];
    startInfo.outNodes.push_back(&endInfo);
    endInfo.info.waitDstStageMask.push_back(waitMask);
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
        node->node->preSubmit(index);
    }

    for (size_t i = 0; i < m_nodeList.size(); i++) {
        auto node = m_nodeList[i];
        node->info.commandBuffers.clear();

        m_fences[index][i].wait();
        m_fences[index][i].reset();

        auto commandBuffers = node->node->getCommands(index);

        for (auto commandBuffer : commandBuffers) {
            node->info.commandBuffers.push_back(*commandBuffer);
        }

        node->queue->submit({ node->info }, &m_fences[index][i]);
    }

    for (auto node : m_nodeList) {
        node->node->postSubmit(index);
    }

    m_frame++;
}

size_t QueueGraph::completedFrames() const {
    if (m_frameCount > m_frame) return 0;
    return m_frame - m_frameCount;
}