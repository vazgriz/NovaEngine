#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <NovaEngine/NovaEngine.h>
#include <glm/glm.hpp>
#include <fstream>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static std::vector<vk::VertexInputAttributeDescription> getAttributes() {
        vk::VertexInputAttributeDescription att1 = {};
        att1.binding = 0;
        att1.format = vk::Format::R32G32B32_Sfloat;
        att1.location = 0;
        att1.offset = 0;

        vk::VertexInputAttributeDescription att2 = {};
        att2.binding = 0;
        att2.format = vk::Format::R32G32B32_Sfloat;
        att2.location = 1;
        att2.offset = sizeof(glm::vec3);

        return { att1, att2 };
    }

    static std::vector<vk::VertexInputBindingDescription> getBindings() {
        vk::VertexInputBindingDescription binding = {};
        binding.binding = 0;
        binding.inputRate = vk::VertexInputRate::Vertex;
        binding.stride = sizeof(Vertex);

        return { binding };
    }
};

std::vector<Vertex> vertices = {
    { { -1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { {  0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

std::vector<char> loadFile(const std::string& fileName) {
    std::ifstream file = std::ifstream(fileName, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + fileName);
    }

    std::vector<char> result;
    result.resize(file.tellg());

    file.seekg(0);
    file.read(result.data(), result.size());

    return result;
}

vk::ShaderModule loadShader(vk::Device& device, const std::string& fileName) {
    vk::ShaderModuleCreateInfo info = {};
    info.code = loadFile(fileName);

    return vk::ShaderModule(device, info);
}

class TestNode : public Nova::QueueNode {
public:
    TestNode(const vk::Queue& queue, vk::Swapchain& swapchain, Nova::BufferAllocator& allocator, Nova::TransferNode& transferNode) : QueueNode(queue) {
        m_allocator = &allocator;
        m_transferNode = &transferNode;
        createSemaphores();
        createVertexBuffer();
        createPipelineLayout();
        setSwapchain(swapchain);
    }

    void setSwapchain(vk::Swapchain& swapchain) {
        m_swapchain = &swapchain;
        createRenderPass();
        createImageViews();
        createFramebuffers();
        createPipeline();
    }

    vk::Semaphore& acquireSemaphore() { return *m_acquireSemaphore; }
    vk::Semaphore& renderSemaphore() { return *m_renderSemaphore; }

private:
    vk::Swapchain* m_swapchain;
    Nova::BufferAllocator* m_allocator;
    Nova::TransferNode* m_transferNode;
    std::unique_ptr<vk::Semaphore> m_acquireSemaphore;
    std::unique_ptr<vk::Semaphore> m_renderSemaphore;
    uint32_t m_index;
    std::vector<const vk::CommandBuffer*> m_commands;
    std::unique_ptr<vk::RenderPass> m_renderPass;
    std::vector<vk::ImageView> m_imageViews;
    std::vector<vk::Framebuffer> m_framebuffers;
    std::unique_ptr<Nova::Buffer> m_vertexBuffer;
    std::unique_ptr<vk::PipelineLayout> m_pipelineLayout;
    std::unique_ptr<vk::Pipeline> m_pipeline;

    void createSemaphores() {
        vk::SemaphoreCreateInfo info = {};

        m_acquireSemaphore = std::make_unique<vk::Semaphore>(queue().device(), info);
        m_renderSemaphore = std::make_unique<vk::Semaphore>(queue().device(), info);
    }

    void preSubmit(size_t index) override {
        m_swapchain->acquireNextImage(~0, m_acquireSemaphore.get(), nullptr, m_index);
    }

    std::vector<const vk::CommandBuffer*>& getCommands(size_t index) override {
        vk::CommandBuffer& commandBuffer = commandBuffers()[index];
        commandBuffer.reset(vk::CommandBufferResetFlags::None);

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlags::OneTimeSubmit;

        commandBuffer.begin(beginInfo);

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_renderPass.get();
        renderPassInfo.framebuffer = &m_framebuffers[m_index];
        renderPassInfo.renderArea.extent = m_swapchain->extent();
        renderPassInfo.clearValues = { {} };

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::Inline);

        commandBuffer.bindVertexBuffers(0, { m_vertexBuffer->resource() }, { 0 });
        commandBuffer.bindPipeline(vk::PipelineBindPoint::Graphics, *m_pipeline);
        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRenderPass();

        commandBuffer.end();

        m_commands.clear();
        m_commands.push_back(&commandBuffer);

        return m_commands;
    }

    void postSubmit(size_t index) override {
        vk::PresentInfo info = {};
        info.swapchains = { *m_swapchain };
        info.imageIndices = { m_index };
        info.waitSemaphores = { *m_renderSemaphore };

        queue().present(info);
    }

    void createRenderPass() {
        vk::AttachmentDescription attachment = {};
        attachment.format = m_swapchain->format();
        attachment.initialLayout = vk::ImageLayout::Undefined;
        attachment.finalLayout = vk::ImageLayout::PresentSrcKhr;
        attachment.loadOp = vk::AttachmentLoadOp::DontCare;
        attachment.storeOp = vk::AttachmentStoreOp::Store;
        attachment.samples = vk::SampleCountFlags::_1;

        vk::AttachmentReference attachmentRef = {};
        attachmentRef.attachment = 0;
        attachmentRef.layout = vk::ImageLayout::ColorAttachmentOptimal;

        vk::SubpassDescription subpass = {};
        subpass.colorAttachments = { attachmentRef };
        subpass.pipelineBindPoint = vk::PipelineBindPoint::Graphics;

        vk::RenderPassCreateInfo info = {};
        info.attachments = { attachment };
        info.subpasses = { subpass };

        m_renderPass = std::make_unique<vk::RenderPass>(queue().device(), info);
    }

    void createImageViews() {
        m_imageViews.clear();
        for (size_t i = 0; i < m_swapchain->images().size(); i++) {
            vk::ImageViewCreateInfo info = {};
            info.image = &m_swapchain->images()[i];
            info.format = m_swapchain->format();
            info.viewType = vk::ImageViewType::_2D;
            info.subresourceRange.aspectMask = vk::ImageAspectFlags::Color;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;

            m_imageViews.emplace_back(queue().device(), info);
        }
    }

    void createFramebuffers() {
        m_framebuffers.clear();
        for (size_t i = 0; i < m_swapchain->images().size(); i++) {
            vk::FramebufferCreateInfo info = {};
            info.renderPass = m_renderPass.get();
            info.attachments = { m_imageViews[i] };
            info.width = m_swapchain->extent().width;
            info.height = m_swapchain->extent().height;
            info.layers = 1;

            m_framebuffers.emplace_back(queue().device(), info);
        }
    }

    void createVertexBuffer() {
        vk::BufferCreateInfo info = {};
        info.size = vertices.size() * sizeof(Vertex);
        info.usage = vk::BufferUsageFlags::VertexBuffer | vk::BufferUsageFlags::TransferDst;

        m_vertexBuffer = std::make_unique<Nova::Buffer>(m_allocator->allocate(info, vk::MemoryPropertyFlags::DeviceLocal, {}));

        vk::BufferCopy copy = {};
        copy.size = vertices.size() * sizeof(Vertex);
        m_transferNode->transfer(vertices.data(), *m_vertexBuffer, copy);
    }

    void createPipelineLayout() {
        vk::PipelineLayoutCreateInfo info = {};

        m_pipelineLayout = std::make_unique<vk::PipelineLayout>(queue().device(), info);
    }

    void createPipeline() {
        vk::ShaderModule vertShader = loadShader(queue().device(), "shader.vert.spv");
        vk::ShaderModule fragShader = loadShader(queue().device(), "shader.frag.spv");

        vk::PipelineShaderStageCreateInfo vertStage = {};
        vertStage.stage = vk::ShaderStageFlags::Vertex;
        vertStage.module = &vertShader;
        vertStage.name = "main";

        vk::PipelineShaderStageCreateInfo fragStage = {};
        fragStage.stage = vk::ShaderStageFlags::Fragment;
        fragStage.module = &fragShader;
        fragStage.name = "main";

        vk::PipelineVertexInputStateCreateInfo vertexInput = {};
        vertexInput.vertexAttributeDescriptions = Vertex::getAttributes();
        vertexInput.vertexBindingDescriptions = Vertex::getBindings();

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.topology = vk::PrimitiveTopology::TriangleList;

        vk::Viewport viewport = {};
        viewport.width = static_cast<float>(m_swapchain->extent().width);
        viewport.height = static_cast<float>(m_swapchain->extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor = {};
        scissor.extent = m_swapchain->extent();

        vk::PipelineViewportStateCreateInfo viewportState = {};
        viewportState.viewports = { viewport };
        viewportState.scissors = { scissor };

        vk::PipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.polygonMode = vk::PolygonMode::Fill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlags::Back;
        rasterizer.frontFace = vk::FrontFace::Clockwise;

        vk::PipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.rasterizationSamples = vk::SampleCountFlags::_1;

        vk::PipelineColorBlendAttachmentState blendAttachment = {};
        blendAttachment.colorWriteMask = vk::ColorComponentFlags::R |
                                         vk::ColorComponentFlags::G |
                                         vk::ColorComponentFlags::B |
                                         vk::ColorComponentFlags::A;

        vk::PipelineColorBlendStateCreateInfo blending = {};
        blending.attachments = { blendAttachment };

        vk::GraphicsPipelineCreateInfo info = {};
        info.stages = { vertStage, fragStage };
        info.vertexInputState = &vertexInput;
        info.inputAssemblyState = &inputAssembly;
        info.viewportState = &viewportState;
        info.rasterizationState = &rasterizer;
        info.multisampleState = &multisampling;
        info.colorBlendState = &blending;
        info.layout = m_pipelineLayout.get();
        info.renderPass = m_renderPass.get();
        info.subpass = 0;

        m_pipeline = std::make_unique<vk::GraphicsPipeline>(queue().device(), info);
    }
};

int main() {
    glfwInit();

    {
        Nova::Renderer renderer = Nova::Renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });

        for (auto& physicalDevice : renderer.validDevices()) {
            renderer.createDevice(*physicalDevice, {}, nullptr);
            break;
        }

        Nova::Engine engine = Nova::Engine(renderer);

        std::unique_ptr<Nova::BufferAllocator> allocator;

        Nova::Window window = Nova::Window(engine, 800, 600);
        Nova::QueueGraph graph = Nova::QueueGraph(engine, window.swapchain().images().size());

        allocator = std::make_unique<Nova::BufferAllocator>(engine, graph, 256 * 1024 * 1024);

        auto& transferNode = graph.addNode<Nova::TransferNode>(engine, *renderer.graphicsQueue(), graph, 64 * 1024 * 1024);
        auto& node = graph.addNode<TestNode>(*renderer.graphicsQueue(), window.swapchain(), *allocator, transferNode);

        graph.addEdge(transferNode, node, vk::PipelineStageFlags::VertexShader);
        graph.addExternalWait(node, node.acquireSemaphore(), vk::PipelineStageFlags::ColorAttachmentOutput);
        graph.addExternalSignal(node, node.renderSemaphore());
        graph.bake();

        auto slot1 = window.onSwapchainChanged().connectMember(node, &TestNode::setSwapchain);
        auto slot2 = window.onSwapchainChanged().connect([&](vk::Swapchain& swapchain) {
            graph.setFrames(swapchain.images().size());
        });

        while (!window.shouldClose()) {
            glfwPollEvents();
            if (window.iconified()) {
                glfwWaitEvents();
            } else {
                window.update();
                graph.submit();
                allocator->update();
            }
        }
    }

    glfwTerminate();
    return 0;
}