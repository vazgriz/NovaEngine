#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <NovaEngine/NovaEngine.h>

class TestNode : public Nova::QueueNode {
public:
    TestNode(const vk::Queue& queue, vk::Swapchain& swapchain) : QueueNode(queue) {
        setSwapchain(swapchain);
        createSemaphores();
        createCommandPool();
    }

    void setSwapchain(vk::Swapchain& swapchain) {
        m_swapchain = &swapchain;
        createRenderPass();
        createImageViews();
        createFramebuffers();
    }

    vk::Semaphore& acquireSemaphore() { return *m_acquireSemaphore; }
    vk::Semaphore& renderSemaphore() { return *m_renderSemaphore; }

private:
    vk::Swapchain* m_swapchain;
    std::unique_ptr<vk::Semaphore> m_acquireSemaphore;
    std::unique_ptr<vk::Semaphore> m_renderSemaphore;
    uint32_t m_index;
    std::unique_ptr<vk::CommandPool> m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<const vk::CommandBuffer*> m_commands;
    std::unique_ptr<vk::RenderPass> m_renderPass;
    std::vector<vk::ImageView> m_imageViews;
    std::vector<vk::Framebuffer> m_framebuffers;

    void createSemaphores() {
        vk::SemaphoreCreateInfo info = {};

        m_acquireSemaphore = std::make_unique<vk::Semaphore>(queue().device(), info);
        m_renderSemaphore = std::make_unique<vk::Semaphore>(queue().device(), info);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo info = {};
        info.queueFamilyIndex = queue().familyIndex();
        info.flags = vk::CommandPoolCreateFlags::ResetCommandBuffer;

        m_commandPool = std::make_unique<vk::CommandPool>(queue().device(), info);

        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = m_commandPool.get();
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_swapchain->images().size());

        m_commandBuffers = m_commandPool->allocate(allocInfo);
    }

    void preSubmit() override {
        m_index = m_swapchain->acquireNextImage(~0, m_acquireSemaphore.get(), nullptr);
    }

    std::vector<const vk::CommandBuffer*>& getCommands() override {
        vk::CommandBuffer& commandBuffer = m_commandBuffers[m_index];
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
        commandBuffer.endRenderPass();

        commandBuffer.end();

        m_commands.clear();
        m_commands.push_back(&commandBuffer);

        return m_commands;
    }

    void postSubmit() override {
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

        Nova::Window window = Nova::Window(engine, 800, 600);
        Nova::QueueGraph graph = Nova::QueueGraph(engine, window.swapchain().images().size());
        auto& node = graph.addNode<TestNode>(*renderer.graphicsQueue(), window.swapchain());
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
            }
        }
    }

    glfwTerminate();
    return 0;
}