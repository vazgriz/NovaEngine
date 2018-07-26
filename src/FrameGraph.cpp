#include "NovaEngine/FrameGraph.h"
#include "NovaEngine/DirectedAcyclicGraph.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

BufferUsage::BufferUsage(FrameNode* node, vk::AccessFlags accessMask) {
    m_node = node;
    m_accessMask = accessMask;
}

void BufferUsage::add(const Buffer& buffer, size_t offset, size_t size) {
    if (!m_node->addBuffer(buffer.resource(), { &buffer.resource(), offset, size, this })) {
        if (m_node->m_bufferMap[&buffer.resource()].usage == this) return;
        throw std::runtime_error("Buffer already used by this RenderNode");
    }
}

ImageUsage::ImageUsage(FrameNode* node, vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_node = node;
    m_accessMask = accessMask;
    m_layout = layout;
}

void ImageUsage::add(const Image& image, vk::ImageSubresourceRange range) {
    if (!m_node->addImage(image.resource(), { &image.resource(), range, this })) {
        if (m_node->m_imageMap[&image.resource()].usage == this) return;
        throw std::runtime_error("Image already used by this RenderNode");
    }
}

FrameNode::FrameNode(const vk::Queue& queue, vk::PipelineStageFlags sourceStages, vk::PipelineStageFlags destStages) {
    m_queue = &queue;
    m_family = m_queue->familyIndex();
    m_sourceStages = sourceStages;
    m_destStages = destStages;

    createCommandPool();
}

void FrameNode::createCommandPool() {
    vk::CommandPoolCreateInfo info = {};
    info.queueFamilyIndex = m_family;
    info.flags = vk::CommandPoolCreateFlags::ResetCommandBuffer;

    m_pool = std::make_unique<vk::CommandPool>(m_queue->device(), info);
}

void FrameNode::createCommandBuffers(size_t frames) {
    vk::CommandBufferAllocateInfo info = {};
    info.commandPool = m_pool.get();
    info.commandBufferCount = static_cast<uint32_t>(frames);

    m_commandBuffers = m_pool->allocate(info);
}

BufferUsage& FrameNode::addBufferUsage(vk::AccessFlags accessMask) {
    m_bufferUsages.emplace_back(std::make_unique<BufferUsage>(this, accessMask));
    return *m_bufferUsages.back();
}

ImageUsage& FrameNode::addImageUsage(vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_imageUsages.emplace_back(std::make_unique<ImageUsage>(this, accessMask, layout));
    return *m_imageUsages.back();
}

bool FrameNode::addBuffer(vk::Buffer& buffer, BufferUsage::Instance usage) {
    return m_bufferMap.insert({ &buffer, usage }).second;
}

bool FrameNode::addImage(vk::Image& image, ImageUsage::Instance usage) {
    return m_imageMap.insert({ &image, usage }).second;
}

void FrameNode::clearInstances() {
    m_bufferMap.clear();
    m_imageMap.clear();
}

void FrameNode::preRecord(vk::CommandBuffer& commandBuffer) {
    for (auto event : m_inEvents) {
        event->recordDest(commandBuffer);
    }
}

void FrameNode::postRecord(vk::CommandBuffer& commandBuffer) {
    for (auto event : m_outEvents) {
        event->recordSource(commandBuffer);
    }
}

FrameGraph::Group::Group(const vk::Queue& queue) {
    this->queue = &queue;
    this->family = queue.familyIndex();
    this->info = {};
    sourceStages = {};
    destStages = {};
}

void FrameGraph::Group::addNode(FrameNode* node) {
    nodes.push_back(node);
    sourceStages |= node->m_sourceStages;
    destStages |= node->m_destStages;
}

void FrameGraph::Group::submit(size_t frame, size_t index, vk::Fence& fence) {
    info.commandBuffers.clear();

    for (auto node : nodes) {
        auto commands = node->submit(frame, index);
        for (auto command : commands) {
            info.commandBuffers.push_back(*command);
        }
    }

    queue->submit({ info }, &fence);
}

FrameGraph::Event::Event(FrameNode& source, FrameNode& dest) {
    this->source = &source;
    this->dest = &dest;

    if (source.m_family == dest.m_family) {
        vk::EventCreateInfo info = {};

        event = std::make_unique<vk::Event>(source.m_queue->device(), info);
    }
}

void FrameGraph::Event::buildBarriers() {
    sourceBufferBarriers.clear();
    sourceImageBarriers.clear();
    destBufferBarriers.clear();
    destImageBarriers.clear();

    for (auto& sourceInstance : source->m_bufferMap) {
        auto destInstance = dest->m_bufferMap.find(sourceInstance.first);
        if (destInstance != dest->m_bufferMap.end()) {
            vk::BufferMemoryBarrier sourceBarrier = {};
            sourceBarrier.buffer = sourceInstance.first;
            sourceBarrier.offset = sourceInstance.second.offset;
            sourceBarrier.size = sourceInstance.second.size;
            sourceBarrier.srcQueueFamilyIndex = source->m_family;
            sourceBarrier.dstQueueFamilyIndex = dest->m_family;
            sourceBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;

            if (event != nullptr) {
                sourceBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;
            }

            sourceBufferBarriers.push_back(sourceBarrier);

            vk::BufferMemoryBarrier destBarrier = {};
            destBarrier.buffer = destInstance->first;
            destBarrier.offset = destInstance->second.offset;
            destBarrier.size = destInstance->second.size;
            destBarrier.srcQueueFamilyIndex = source->m_family;
            destBarrier.dstQueueFamilyIndex = dest->m_family;

            if (event != nullptr) {
                destBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;
            }

            destBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;

            destBufferBarriers.push_back(destBarrier);
        }
    }

    for (auto& sourceInstance : source->m_imageMap) {
        auto destInstance = dest->m_imageMap.find(sourceInstance.first);
        if (destInstance != dest->m_imageMap.end()) {
            vk::ImageMemoryBarrier sourceBarrier = {};
            sourceBarrier.image = sourceInstance.first;
            sourceBarrier.subresourceRange = sourceInstance.second.range;
            sourceBarrier.srcQueueFamilyIndex = source->m_family;
            sourceBarrier.dstQueueFamilyIndex = dest->m_family;
            sourceBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;

            if (event != nullptr) {
                sourceBarrier.oldLayout = sourceInstance.second.usage->m_layout;    //transition only once if on the same family
                sourceBarrier.newLayout = sourceInstance.second.usage->m_layout;    //so don't transition with this barrier
                sourceBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;
            } else {
                sourceBarrier.oldLayout = sourceInstance.second.usage->m_layout;
                sourceBarrier.newLayout = destInstance->second.usage->m_layout;
            }
            
            sourceImageBarriers.push_back(sourceBarrier);

            vk::ImageMemoryBarrier destBarrier = {};
            destBarrier.image = destInstance->first;
            destBarrier.subresourceRange = destInstance->second.range;
            destBarrier.oldLayout = sourceInstance.second.usage->m_layout;
            destBarrier.newLayout = destInstance->second.usage->m_layout;
            destBarrier.srcQueueFamilyIndex = source->m_family;
            destBarrier.dstQueueFamilyIndex = dest->m_family;

            if (event != nullptr) {
                destBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;
            }

            destBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;

            destImageBarriers.push_back(destBarrier);
        }
    }
}

void FrameGraph::Event::recordSource(vk::CommandBuffer& commandBuffer) {
    if (event != nullptr) {
        commandBuffer.pipelineBarrier(source->m_destStages, dest->m_destStages, {}, {}, sourceBufferBarriers, sourceImageBarriers);
        commandBuffer.setEvent(*event, source->m_destStages);
    } else {
        commandBuffer.pipelineBarrier(source->m_destStages, vk::PipelineStageFlags::BottomOfPipe, {}, {}, sourceBufferBarriers, sourceImageBarriers);
    }
}

void FrameGraph::Event::recordDest(vk::CommandBuffer& commandBuffer) {
    if (event != nullptr) {
        commandBuffer.waitEvents({ *event }, source->m_destStages, dest->m_sourceStages, {}, destBufferBarriers, destImageBarriers);
    } else {
        commandBuffer.pipelineBarrier(vk::PipelineStageFlags::TopOfPipe, dest->m_sourceStages, {}, {}, destBufferBarriers, destImageBarriers);
    }
}

FrameGraph::FrameGraph(Engine& engine) {
    m_engine = &engine;
    m_onFrameCountChanged = std::make_unique<Signal<size_t>>();
}

void FrameGraph::addNode(FrameNode& node) {
    m_nodes.push_back(&node);
}

void FrameGraph::addExternalSignal(FrameNode& node, vk::Semaphore& semaphore) {
    node.m_externalSignals.push_back(&semaphore);
}

void FrameGraph::addExternalWait(FrameNode& node, vk::Semaphore& semaphore, vk::PipelineStageFlags waitMask) {
    node.m_externalWaits.push_back(&semaphore);
    node.m_externalWaitMasks.push_back(waitMask);
}

void FrameGraph::addEdge(FrameNode& source, FrameNode& dest) {
    m_events.emplace_back(std::make_unique<FrameGraph::Event>(source, dest));
    auto& event = *m_events.back();
    source.m_outEvents.push_back(&event);
    dest.m_inEvents.push_back(&event);
}

void FrameGraph::setFrameCount(size_t frames) {
    if (frames == m_frameCount) return;
    internalSetFrames(frames);
}

void FrameGraph::internalSetFrames(size_t frames) {
    m_frameCount = frames;

    for (auto& v : m_fences) {
        if (v.size() > 0) {
            vk::Fence::wait(m_engine->renderer().device(), v, true);
        }
    }

    m_fences.clear();

    for (size_t i = 0; i < frames; i++) {
        m_fences.emplace_back();
        auto& groupFences = m_fences.back();

        for (auto& group : m_groups) {
            vk::FenceCreateInfo info = {};
            info.flags = vk::FenceCreateFlags::Signaled;

            groupFences.emplace_back(m_engine->renderer().device(), info);
        }
    }

    for (auto& node : m_nodeList) {
        node->createCommandBuffers(m_frameCount);
    }
}

void FrameGraph::bake() {
    std::unordered_set<FrameNode*> nodes;
    
    for (auto& node : m_nodes) {
        nodes.insert(node);
    }
    
    m_nodeList = topologicalSort<FrameNode>(nodes, [](const FrameNode* node) {
        std::vector<FrameNode*> results;
        for (auto event : node->m_outEvents) {
            results.push_back(event->dest);
        }
        return results;
    });
    
    createGroups();
    internalSetFrames(m_frameCount);
    createSemaphores();
}

void FrameGraph::createGroups() {
    std::unordered_set<FrameNode*> finished;

    for (auto node : m_nodeList) {
        if (finished.count(node) == 1) continue;

        uint32_t family = node->m_family;
        std::unordered_set<FrameNode*> excluded;
        std::queue<FrameNode*> queue;
        queue.push(node);

        while (!queue.empty()) {
            FrameNode* currentNode = queue.front();
            queue.pop();

            if (excluded.count(currentNode) == 1) continue;

            if (currentNode->m_family != family) {
                excluded.insert(currentNode);
            }

            for (auto outEvent : currentNode->m_outEvents) {
                queue.push(outEvent->dest);
            }
        }

        std::unique_ptr<Group> group = std::make_unique<Group>(node->queue());

        for (size_t j = 0; j < m_nodeList.size(); j++) {
            FrameNode* currentNode = m_nodeList[j];
            if (finished.count(currentNode) == 1) continue;
            if (excluded.count(currentNode) == 1) continue;

            finished.insert(currentNode);

            group->addNode(currentNode);
            currentNode->m_group = group.get();

            for (auto signal : node->m_externalSignals) {
                group->info.signalSemaphores.push_back(*signal);
            }

            for (size_t i = 0; i < node->m_externalWaits.size(); i++) {
                group->info.waitSemaphores.push_back(*node->m_externalWaits[i]);
                group->info.waitDstStageMask.push_back(node->m_externalWaitMasks[i]);
            }
        }

        m_groups.emplace_back(std::move(group));
    }
}

void FrameGraph::createSemaphores() {
    for (auto& group : m_groups) {
        std::unordered_set<Group*> outGroupSet;

        for (auto node : group->nodes) {
            for (auto outEvent : node->m_outEvents) {
                outGroupSet.insert(outEvent->dest->m_group);
            }
        }

        std::vector<Group*> outGroups;
        for (auto outGroup : outGroupSet) {
            outGroups.push_back(outGroup);
        }

        for (auto outGroup : outGroups) {
            vk::SemaphoreCreateInfo info = {};
            m_semaphores.emplace_back(std::make_unique<vk::Semaphore>(m_engine->renderer().device(), info));
            auto& semaphore = *m_semaphores.back();

            group->info.signalSemaphores.push_back(semaphore);
            outGroup->info.waitSemaphores.push_back(semaphore);
            outGroup->info.waitDstStageMask.push_back(group->destStages);
        }
    }
}

size_t FrameGraph::completedFrames() const {
    if (m_frame < m_frameCount) return 0;
    return m_frame - m_frameCount;
}

void FrameGraph::submit() {
    size_t index = m_frame % m_fences.size();

    for (auto node : m_nodeList) {
        node->preSubmit(m_frame);
    }

    for (auto& event : m_events) {
        event->buildBarriers();
    }

    for (size_t i = 0; i < m_groups.size(); i++) {
        auto& group = m_groups[i];

        m_fences[index][i].wait();
        m_fences[index][i].reset();

        group->submit(m_frame, index, m_fences[index][i]);
    }

    for (auto node : m_nodeList) {
        node->postSubmit(m_frame);
        node->clearInstances();
    }

    m_frame++;
}