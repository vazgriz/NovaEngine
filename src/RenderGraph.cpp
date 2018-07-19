#include "NovaEngine/RenderGraph.h"
#include <queue>
#include "NovaEngine/DirectedAcyclicGraph.h"

using namespace Nova;

BufferUsage::BufferUsage(RenderNode* node, vk::AccessFlags accessMask) {
    m_node = node;
    m_accessMask = accessMask;
}

void BufferUsage::add(vk::Buffer& buffer, size_t offset, size_t size) {
    if (m_node->addBuffer(buffer, *this)) {
        m_buffers.push_back({ &buffer, offset,size });
    } else {
        if (m_node->m_bufferMap[&buffer] == this) return;
        throw std::runtime_error("Buffer already used by this RenderNode");
    }
}

void BufferUsage::clear() {
    m_buffers.clear();
}

ImageUsage::ImageUsage(RenderNode* node, vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_node = node;
    m_accessMask = accessMask;
    m_layout = layout;
}

void ImageUsage::add(vk::Image& image, vk::ImageSubresourceRange range) {
    if (m_node->addImage(image, *this)) {
        m_images.push_back({ &image, range });
    } else {
        if (m_node->m_imageMap[&image] == this) return;
        throw std::runtime_error("Image already used by this RenderNode");
    }
}

void ImageUsage::clear() {
    m_images.clear();
}

RenderNode::RenderNode(RenderGraph* graph, const vk::Queue& queue) {
    m_graph = graph;
    m_queue = &queue;
    m_family = queue.familyIndex();
}

BufferUsage& RenderNode::addBufferUsage(vk::AccessFlags accessMask) {
    m_bufferUsages.emplace_back(this, accessMask);
    return m_bufferUsages.back();
}

ImageUsage& RenderNode::addImageUsage(vk::AccessFlags accessMask, vk::ImageLayout layout) {
    m_imageUsages.emplace_back(this, accessMask, layout);
    return m_imageUsages.back();
}

void RenderNode::preRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) {
    buildBarriers();

    for (size_t i = 0; i < m_inEvents.size(); i++) {
        if (m_inEvents[i] != nullptr) {
            commandBuffer.waitEvents({ *m_inEvents[i] }, srcStage, dstStage, {}, {}, {});
            commandBuffer.resetEvent(*m_inEvents[i], dstStage);
        } else {
            commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags::None,
                m_inMemoryBarriers, m_inBufferBarriers, m_inImageBarriers);
        }
    }

    m_inMemoryBarriers.clear();
    m_inBufferBarriers.clear();
    m_inImageBarriers.clear();
    m_bufferMap.clear();
    m_imageMap.clear();

    for (auto& usage : m_bufferUsages) {
        usage.clear();
    }

    for (auto& usage : m_imageUsages) {
        usage.clear();
    }
}

void RenderNode::postRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) {
    for (size_t i = 0; i < m_outEvents.size(); i++) {
        if (m_outEvents[i] != nullptr) {
            commandBuffer.setEvent(*m_outEvents[i], dstStage);
        } else {
            commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags::None,
                m_outMemoryBarriers, m_outBufferBarriers, m_outImageBarriers);
        }
    }

    m_outMemoryBarriers.clear();
    m_outBufferBarriers.clear();
    m_outImageBarriers.clear();
}

bool RenderNode::addBuffer(vk::Buffer& buffer, BufferUsage& usage) {
    return m_bufferMap.insert({ &buffer, &usage }).second;
}

bool RenderNode::addImage(vk::Image& image, ImageUsage& usage) {
    return m_imageMap.insert({ &image, &usage }).second;
}

void RenderNode::buildBarriers() {
    for (auto& usage : m_bufferUsages) {
        for (auto& instance : usage.m_buffers) {
            for (auto in : m_inNodes) {
                auto it = in->m_bufferMap.find(instance.buffer);
                if (it != in->m_bufferMap.end()) {
                    vk::BufferMemoryBarrier barrier = {};
                    barrier.buffer = instance.buffer;
                    barrier.offset = instance.offset;
                    barrier.size = instance.size;
                    barrier.srcAccessMask = it->second->m_accessMask;
                    barrier.dstAccessMask = usage.m_accessMask;
                    barrier.srcQueueFamilyIndex = in->m_family;
                    barrier.dstQueueFamilyIndex = m_family;

                    m_inBufferBarriers.push_back(barrier);
                }
            }

            for (auto out : m_outNodes) {
                auto it = out->m_bufferMap.find(instance.buffer);
                if (it != out->m_bufferMap.end()) {
                    vk::BufferMemoryBarrier barrier = {};
                    barrier.buffer = instance.buffer;
                    barrier.offset = instance.offset;
                    barrier.size = instance.size;
                    barrier.srcAccessMask = usage.m_accessMask;
                    barrier.dstAccessMask = it->second->m_accessMask;
                    barrier.srcQueueFamilyIndex = m_family;
                    barrier.dstQueueFamilyIndex = out->m_family;

                    m_outBufferBarriers.push_back(barrier);
                }
            }
        }
    }

    for (auto& usage : m_imageUsages) {
        for (auto& instance : usage.m_images) {
            for (auto in : m_inNodes) {
                auto it = in->m_imageMap.find(instance.image);
                if (it != in->m_imageMap.end()) {
                    vk::ImageMemoryBarrier barrier = {};
                    barrier.image = instance.image;
                    barrier.subresourceRange = instance.range;
                    barrier.srcAccessMask = it->second->m_accessMask;
                    barrier.dstAccessMask = usage.m_accessMask;
                    barrier.oldLayout = it->second->m_layout;
                    barrier.newLayout = usage.m_layout;
                    barrier.srcQueueFamilyIndex = in->m_family;
                    barrier.dstQueueFamilyIndex = m_family;

                    m_inImageBarriers.push_back(barrier);
                }
            }

            for (auto out : m_outNodes) {
                if (out->m_family != m_family) {
                    auto it = out->m_imageMap.find(instance.image);
                    if (it != out->m_imageMap.end()) {
                        vk::ImageMemoryBarrier barrier = {};
                        barrier.image = instance.image;
                        barrier.subresourceRange = instance.range;
                        barrier.srcAccessMask = usage.m_accessMask;
                        barrier.dstAccessMask = it->second->m_accessMask;
                        barrier.oldLayout = usage.m_layout;
                        barrier.newLayout = it->second->m_layout;
                        barrier.srcQueueFamilyIndex = m_family;
                        barrier.dstQueueFamilyIndex = out->m_family;

                        m_outImageBarriers.push_back(barrier);
                    }
                }
            }
        }
    }
}

RenderGraph::RenderGraph(Engine& engine) {
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