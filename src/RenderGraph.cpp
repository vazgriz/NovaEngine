#include "NovaEngine/RenderGraph.h"
#include <queue>
#include "NovaEngine/DirectedAcyclicGraph.h"

using namespace Nova;

RenderNode::RenderNode(const vk::Queue& queue) {
    m_queue = &queue;
    m_family = queue.familyIndex();
}

void RenderNode::addExternalWait(vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask) {
    m_externalWaits.push_back(&semaphore);
    m_externalStageMasks.push_back(stageMask);
}

void RenderNode::addExternalSignal(vk::Semaphore& semaphore) {
    m_externalSignals.push_back(&semaphore);
}

RenderGraph::RenderGraph(Engine& engine, size_t frames) {
    m_engine = &engine;

    setFrames(frames);
}

RenderGraph::~RenderGraph() {
    for (auto& v : m_fences) {
        for (auto& fence : v) {
            fence.wait();
        }
    }
}

void RenderGraph::setFrames(size_t frames) {
    m_fences.clear();

    for (size_t i = 0; i < frames; i++) {
        m_fences.emplace_back();
        auto& groupFences = m_fences.back();

        for (size_t j = 0; j < m_groups.size(); j++) {
            vk::FenceCreateInfo info = {};
            info.flags = vk::FenceCreateFlags::Signaled;

            groupFences.emplace_back(m_engine->renderer().device(), info);
        }
    }
}

void RenderGraph::addEdge(RenderNode& source, RenderNode& dest) {
    source.m_outNodes.push_back(&dest);
}

void RenderGraph::bake() {
    m_nodeList = topologicalSort<RenderNode>(m_nodeSet, [](const RenderNode* node) {
        return node->m_outNodes;
    });

    createGroups();
    setFrames(m_fences.size());
}

void RenderGraph::createGroups() {
    m_groups.clear();

    std::unordered_set<RenderNode*> finished;

    for (size_t i = 0; i < m_nodeList.size(); i++) {
        RenderNode* node = m_nodeList[i];

        if (finished.count(node) == 1) continue;

        uint32_t family = node->m_family;
        std::unordered_set<RenderNode*> excluded;
        std::queue<RenderNode*> queue;
        queue.push(node);

        while (!queue.empty()) {
            RenderNode* currentNode = queue.front();
            queue.pop();

            if (excluded.count(currentNode) == 1) continue;
            if (currentNode->m_family != family) {
                excluded.insert(currentNode);

                for (auto outNode : currentNode->m_outNodes) {
                    queue.push(outNode);
                }
            }
        }

        RenderGroup group;
        group.family = family;
        group.queue = node->m_queue;

        for (size_t j = 0; j < m_nodeList.size(); j++) {
            RenderNode* currentNode = m_nodeList[j];
            if (finished.count(currentNode) == 1) continue;
            if (excluded.count(currentNode) == 1) continue;

            group.nodes.push_back(currentNode);

            for (size_t k = 0; k < currentNode->m_externalWaits.size(); k++) {
                group.info.waitSemaphores.push_back(*currentNode->m_externalWaits[k]);
                group.info.waitDstStageMask.push_back(currentNode->m_externalStageMasks[k]);
            }

            for (auto semaphore : currentNode->m_externalSignals) {
                group.info.signalSemaphores.push_back(*semaphore);
            }
        }

        m_groups.emplace_back(std::move(group));
    }
}

void RenderGraph::submit() {
    size_t index = m_frame % m_fences.size();

    for (auto& group : m_groups) {
        for (auto node : group.nodes) {
            node->preRender();
        }
    }

    for (size_t i = 0; i < m_groups.size(); i++) {
        auto& group = m_groups[i];
        group.info.commandBuffers.clear();

        m_fences[index][i].wait();
        m_fences[index][i].reset();

        for (auto node : group.nodes) {
            auto commandBuffers = node->render();

            for (auto commandBuffer : commandBuffers) {
                group.info.commandBuffers.push_back(*commandBuffer);
            }

            group.queue->submit(group.info, &m_fences[index][i]);
        }
    }

    for (auto& group : m_groups) {
        for (auto node : group.nodes) {
            node->postRender();
        }
    }
}