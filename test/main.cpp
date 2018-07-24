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
    TestNode(const vk::Queue& queue, vk::Swapchain& swapchain, Nova::BufferAllocator& allocator, Nova::TransferNode& transferNode, Nova::RenderGraph& renderGraph, Nova::Camera& camera) : QueueNode(queue) {
        m_allocator = &allocator;
        m_transferNode = &transferNode;
        m_camera = &camera;

        m_renderNode = &renderGraph.addNode(queue);
        m_bufferUsage = &m_renderNode->addBufferUsage(vk::AccessFlags::VertexAttributeRead);

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
    Nova::RenderNode& renderNode() const { return *m_renderNode; }
    Nova::BufferUsage& bufferUsage() const { return *m_bufferUsage; }

private:
    vk::Swapchain* m_swapchain;
    Nova::BufferAllocator* m_allocator;
    Nova::TransferNode* m_transferNode;
    Nova::RenderNode* m_renderNode;
    Nova::BufferUsage* m_bufferUsage;
    Nova::Camera* m_camera;
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

    void preSubmit(size_t frame) override {
        m_swapchain->acquireNextImage(~0, m_acquireSemaphore.get(), nullptr, m_index);
        m_vertexBuffer->registerUsage(frame);
    }

    std::vector<const vk::CommandBuffer*>& getCommands(size_t frame, size_t index) override {
        vk::CommandBuffer& commandBuffer = commandBuffers()[index];
        commandBuffer.reset(vk::CommandBufferResetFlags::None);

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlags::OneTimeSubmit;

        commandBuffer.begin(beginInfo);

        m_renderNode->preRecord(commandBuffer, vk::PipelineStageFlags::TopOfPipe, vk::PipelineStageFlags::VertexInput);

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_renderPass.get();
        renderPassInfo.framebuffer = &m_framebuffers[m_index];
        renderPassInfo.renderArea.extent = m_swapchain->extent();
        renderPassInfo.clearValues = { {} };

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::Inline);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::Graphics, *m_pipeline);
        commandBuffer.bindVertexBuffers(0, { m_vertexBuffer->resource() }, { 0 });
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::Graphics, *m_pipelineLayout, 0, { m_camera->descriptor() }, {});
        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRenderPass();

        m_renderNode->postRecord(commandBuffer, vk::PipelineStageFlags::ColorAttachmentOutput, vk::PipelineStageFlags::BottomOfPipe);

        commandBuffer.end();

        m_commands.clear();
        m_commands.push_back(&commandBuffer);

        return m_commands;
    }

    void postSubmit(size_t frame) override {
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
        m_bufferUsage->add(m_vertexBuffer->resource(), 0, copy.size);
    }

    void createPipelineLayout() {
        vk::PipelineLayoutCreateInfo info = {};
        info.setLayouts = { m_camera->layout() };

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

        Nova::BufferAllocator allocator = Nova::BufferAllocator(engine, 256 * 1024 * 1024);

        Nova::Window window = Nova::Window(engine, 800, 600);
        Nova::QueueGraph& graph = engine.queueGraph();

        Nova::RenderGraph& renderGraph = engine.renderGraph();

        auto transferNode = Nova::TransferNode(engine, renderer.transferQueue(), graph, renderGraph, 64 * 1024 * 1024);

        Nova::CameraManager cameraManager = Nova::CameraManager(transferNode);
        Nova::PerspectiveCamera camera = Nova::PerspectiveCamera(engine, cameraManager, allocator, { window.width(), window.height() }, 90.0f);
        camera.setPosition({ 0, 0, 1 });
        camera.setRotation({ 1, 0, 0, 0 });
        Nova::FreeCam freeCam = Nova::FreeCam(window, camera, 0.25f);

        auto node = TestNode(renderer.graphicsQueue(), window.swapchain(), allocator, transferNode, renderGraph, camera);

        graph.addNode(transferNode);
        graph.addNode(node);
        graph.addEdge(transferNode, node, vk::PipelineStageFlags::VertexInput);
        graph.addExternalWait(node, node.acquireSemaphore(), vk::PipelineStageFlags::ColorAttachmentOutput);
        graph.addExternalSignal(node, node.renderSemaphore());
        graph.bake();

        renderGraph.addEdge(transferNode.renderNode(), node.renderNode());
        renderGraph.bake();

        auto slot1 = window.onSwapchainChanged().connect([&](vk::Swapchain& swapchain) {
            graph.setFrames(swapchain.images().size());
            node.setSwapchain(swapchain);
            camera.setSize({ swapchain.extent().width, swapchain.extent().height });
        });

        double time = glfwGetTime();

        while (!window.shouldClose()) {
            double now = glfwGetTime();
            float delta = static_cast<float>(now - time);
            time = now;

            glfwPollEvents();
            window.update();

            if (!window.canRender()) {
                glfwWaitEvents();
            } else {
                freeCam.update(delta);
                cameraManager.update(delta);
                graph.submit();
                engine.memory().update(graph.completedFrames());
                renderGraph.reset();
            }
        }

        graph.wait();
    }

    glfwTerminate();
    return 0;
}