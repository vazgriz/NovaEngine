#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <vector>
#include <unordered_set>

namespace Nova {
    class Engine;
    class RenderNode;

    class BufferUsage {
        friend class RenderNode;

        struct BufferInstance {
            vk::Buffer* buffer;
            size_t offset;
            size_t size;
        };

    public:
        BufferUsage(RenderNode* node, vk::AccessFlags accessMask);

        void add(vk::Buffer& buffer, size_t offset, size_t size);

    private:
        RenderNode* m_node;
        vk::AccessFlags m_accessMask;
        std::vector<BufferInstance> m_buffers;

        void clear();
    };

    class ImageUsage {
        friend class RenderNode;

        struct ImageInstance {
            vk::Image* image;
            vk::ImageSubresourceRange range;
        };

    public:
        ImageUsage(RenderNode* node, vk::AccessFlags accessMask, vk::ImageLayout layout);

        void add(vk::Image& image, vk::ImageSubresourceRange range);

    private:
        RenderNode* m_node;
        vk::AccessFlags m_accessMask;
        vk::ImageLayout m_layout;
        std::vector<ImageInstance> m_images;

        void clear();
    };

    class RenderGraph;

    class RenderNode {
        friend class RenderGraph;
        friend class BufferUsage;
        friend class ImageUsage;

    public:
        RenderNode(RenderGraph* graph, const vk::Queue& queue);
        RenderNode(const RenderNode& other) = delete;
        RenderNode& operator = (const RenderNode& other) = delete;
        RenderNode(RenderNode&& other) = default;
        RenderNode& operator = (RenderNode&& other) = default;
        ~RenderNode() = default;

        BufferUsage& addBufferUsage(vk::AccessFlags accessMask);
        ImageUsage& addImageUsage(vk::AccessFlags accessMask, vk::ImageLayout layout);

        void preRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);
        void postRecord(vk::CommandBuffer& commandBuffer, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);

    private:
        RenderGraph* m_graph;
        const vk::Queue* m_queue;
        uint32_t m_family;
        std::vector<RenderNode*> m_inNodes;
        std::vector<RenderNode*> m_outNodes;
        std::vector<vk::Event*> m_inEvents;
        std::vector<vk::Event*> m_outEvents;

        std::vector<BufferUsage> m_bufferUsages;
        std::vector<ImageUsage> m_imageUsages;
        std::unordered_map<vk::Buffer*, BufferUsage*> m_bufferMap;
        std::unordered_map<vk::Image*, ImageUsage*> m_imageMap;

        std::vector<vk::MemoryBarrier> m_inMemoryBarriers;
        std::vector<vk::BufferMemoryBarrier> m_inBufferBarriers;
        std::vector<vk::ImageMemoryBarrier> m_inImageBarriers;
        std::vector<vk::MemoryBarrier> m_outMemoryBarriers;
        std::vector<vk::BufferMemoryBarrier> m_outBufferBarriers;
        std::vector<vk::ImageMemoryBarrier> m_outImageBarriers;

        bool addBuffer(vk::Buffer& buffer, BufferUsage& usage);
        bool addImage(vk::Image& image, ImageUsage& usage);
        void buildBarriers();
        void reset();
    };

    class RenderGraph {
        friend class RenderNode;
    public:
        RenderGraph(Engine& engine);
        RenderGraph(const RenderGraph& other) = delete;
        RenderGraph& operator = (const RenderGraph& other) = delete;
        RenderGraph(RenderGraph&& other) = default;
        RenderGraph& operator = (RenderGraph&& other) = default;

        RenderNode& addNode(const vk::Queue& queue);

        void addEdge(RenderNode& source, RenderNode& dest);
        void bake();
        void reset();

    private:
        Engine* m_engine;
        std::vector<std::unique_ptr<RenderNode>> m_nodes;
        std::unordered_set<RenderNode*> m_nodeSet;
        std::vector<RenderNode*> m_nodeList;
        std::vector<std::unique_ptr<vk::Event>> m_events;

        vk::Event& createEvent(const vk::Queue& queue);
    };
}