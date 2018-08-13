#include "NovaEngine/FrameGraph.h"
#include "NovaEngine/DirectedAcyclicGraph.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

BufferUsage::BufferUsage(FrameNode* node, vk::PipelineStageFlags stageMask, vk::AccessFlags accessMask) {
    m_node = node;
    m_stageMask = stageMask;
    m_accessMask = accessMask;
}

void BufferUsage::add(const Buffer& buffer, size_t offset, size_t size) {
    if (!m_node->addBuffer(buffer.resource(), { &buffer.resource(), offset, size, this })) {
        if (m_node->m_bufferMap[&buffer.resource()].usage == this) return;
        throw std::runtime_error("Buffer already used by this RenderNode");
    }
}

ImageUsage::ImageUsage(FrameNode* node, vk::PipelineStageFlags stageMask, vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_node = node;
    m_stageMask = stageMask;
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

    m_selfSync = std::make_unique<vk::Semaphore>(m_queue->device(), vk::SemaphoreCreateInfo{});
    m_submitInfo.signalSemaphores.push_back(*m_selfSync);
    m_submitInfo.waitSemaphores.push_back(*m_selfSync);
    m_submitInfo.waitDstStageMask.push_back(m_sourceStages);

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

BufferUsage& FrameNode::addBufferUsage(vk::PipelineStageFlags stageMask, vk::AccessFlags accessMask) {
    m_bufferUsages.emplace_back(std::make_unique<BufferUsage>(this, stageMask, accessMask));
    return *m_bufferUsages.back();
}

ImageUsage& FrameNode::addImageUsage(vk::PipelineStageFlags stageMask, vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_imageUsages.emplace_back(std::make_unique<ImageUsage>(this, stageMask, accessMask, layout));
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

void FrameNode::submit(size_t frame, size_t index, vk::Fence& fence) {
    m_submitInfo.commandBuffers.clear();
    auto commandBuffers = submit(frame, index);

    for (auto commandBuffer : commandBuffers) {
        m_submitInfo.commandBuffers.push_back(*commandBuffer);
    }

    m_queue->submit({ m_submitInfo }, &fence);
}

void FrameNode::addExternalWait(vk::Semaphore& semaphore, vk::PipelineStageFlags stageMask) {
    m_submitInfo.waitSemaphores.push_back(semaphore);
    m_submitInfo.waitDstStageMask.push_back(stageMask);
}

void FrameNode::addExternalSignal(vk::Semaphore& semaphore) {
    m_submitInfo.signalSemaphores.push_back(semaphore);
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

FrameGraph::Edge::Edge(FrameNode& source, FrameNode& dest) {
    this->source = &source;
    this->dest = &dest;

    if (source.m_family == dest.m_family) {
        vk::EventCreateInfo info = {};

        event = std::make_unique<vk::Event>(source.m_queue->device(), info);
    } else {
        vk::SemaphoreCreateInfo info = {};

        semaphore = std::make_unique<vk::Semaphore>(source.m_queue->device(), info);

        source.m_submitInfo.signalSemaphores.push_back(*semaphore);
        dest.m_submitInfo.waitSemaphores.push_back(*semaphore);
        dest.m_submitInfo.waitDstStageMask.push_back(source.m_destStages);
    }
}

void FrameGraph::Edge::buildBarriers() {
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

            sourceBufferBarriers.push_back({ sourceBarrier, sourceInstance.second.usage->m_stageMask, destInstance->second.usage->m_stageMask });

            if (event != nullptr) {
                vk::BufferMemoryBarrier destBarrier = {};
                destBarrier.buffer = destInstance->first;
                destBarrier.offset = destInstance->second.offset;
                destBarrier.size = destInstance->second.size;
                destBarrier.srcQueueFamilyIndex = source->m_family;
                destBarrier.dstQueueFamilyIndex = dest->m_family;
                destBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;
                destBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;

                destBufferBarriers.push_back({ destBarrier, sourceInstance.second.usage->m_stageMask, destInstance->second.usage->m_stageMask });
            }
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
            
            sourceImageBarriers.push_back({ sourceBarrier, sourceInstance.second.usage->m_stageMask, destInstance->second.usage->m_stageMask });

            if (event != nullptr) {
                vk::ImageMemoryBarrier destBarrier = {};
                destBarrier.image = destInstance->first;
                destBarrier.subresourceRange = destInstance->second.range;
                destBarrier.oldLayout = sourceInstance.second.usage->m_layout;
                destBarrier.newLayout = destInstance->second.usage->m_layout;
                destBarrier.srcQueueFamilyIndex = source->m_family;
                destBarrier.dstQueueFamilyIndex = dest->m_family;
                destBarrier.srcAccessMask = sourceInstance.second.usage->m_accessMask;
                destBarrier.dstAccessMask = destInstance->second.usage->m_accessMask;

                destImageBarriers.push_back({ destBarrier, sourceInstance.second.usage->m_stageMask, destInstance->second.usage->m_stageMask });
            }
        }
    }
}

void FrameGraph::Edge::recordSource(vk::CommandBuffer& commandBuffer) {
    if (event != nullptr) {
        for (auto& barrier : sourceBufferBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, barrier.dest, {}, {}, { barrier.barrier }, {});
        }
        for (auto& barrier : sourceImageBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, barrier.dest, {}, {}, {},  { barrier.barrier });
        }
        commandBuffer.setEvent(*event, source->m_destStages);
    } else {
        for (auto& barrier : sourceBufferBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, vk::PipelineStageFlags::BottomOfPipe, {}, {}, { barrier.barrier }, {});
        }
        for (auto& barrier : sourceImageBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, vk::PipelineStageFlags::BottomOfPipe, {}, {}, {}, { barrier.barrier });
        }
    }
}

void FrameGraph::Edge::recordDest(vk::CommandBuffer& commandBuffer) {
    if (event != nullptr) {
        for (auto& barrier : destBufferBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, barrier.dest, {}, {}, { barrier.barrier }, {});
        }
        for (auto& barrier : destImageBarriers) {
            commandBuffer.pipelineBarrier(barrier.source, barrier.dest, {}, {}, {}, { barrier.barrier });
        }
    } else {
        for (auto& barrier : destBufferBarriers) {
            commandBuffer.pipelineBarrier(vk::PipelineStageFlags::TopOfPipe, barrier.dest, {}, {}, { barrier.barrier }, {});
        }
        for (auto& barrier : destImageBarriers) {
            commandBuffer.pipelineBarrier(vk::PipelineStageFlags::TopOfPipe, barrier.dest, {}, {}, {}, { barrier.barrier });
        }
    }
}

FrameGraph::FrameGraph(Engine& engine, size_t frameCount) {
    m_engine = &engine;
    m_onFrameCountChanged = std::make_unique<Signal<size_t>>();
    setFrames(frameCount);
}

void FrameGraph::addNode(FrameNode& node) {
    m_nodes.push_back(&node);
    node.m_graph = this;
}

void FrameGraph::addEdge(FrameNode& source, FrameNode& dest) {
    m_edges.emplace_back(std::make_unique<FrameGraph::Edge>(source, dest));
    auto& event = *m_edges.back();
    source.m_outEvents.push_back(&event);
    dest.m_inEvents.push_back(&event);
}

void FrameGraph::setFrames(size_t frames) {
    m_frameCount = frames;

    for (auto& v : m_fences) {
        if (v.size() > 0) {
            vk::Fence::wait(m_engine->renderer().device(), v, true);
        }
    }

    m_fences.clear();

    for (size_t i = 0; i < frames; i++) {
        m_fences.emplace_back();
        auto& fences = m_fences.back();

        for (auto node : m_nodes) {
            vk::FenceCreateInfo info = {};
            info.flags = vk::FenceCreateFlags::Signaled;

            fences.emplace_back(m_engine->renderer().device(), info);
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
    
    setFrames(m_frameCount);
    preSignal();
}

void FrameGraph::preSignal() {
    vk::SubmitInfo info = {};

    for (auto& node : m_nodes) {
        info.signalSemaphores.push_back(*node->m_selfSync);
    }

    m_engine->renderer().graphicsQueue().submit({ info }, nullptr);
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

    for (auto& event : m_edges) {
        event->buildBarriers();
    }

    for (size_t i = 0; i < m_nodeList.size(); i++) {
        auto node = m_nodeList[i];

        m_fences[index][i].wait();
        m_fences[index][i].reset();

        node->submit(m_frame, index, m_fences[index][i]);
    }

    for (auto node : m_nodeList) {
        node->postSubmit(m_frame);
        node->clearInstances();
    }

    m_frame++;
}