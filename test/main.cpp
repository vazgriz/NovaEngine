#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <NovaEngine/NovaEngine.h>

class TestNode : public Nova::RenderNode {
public:
    TestNode(const vk::Queue& queue, vk::Swapchain& swapchain) : RenderNode(queue) {
        setSwapchain(swapchain);
        createSemaphores();
        createCommandPool();
    }

    void setSwapchain(vk::Swapchain& swapchain) {
        m_swapchain = &swapchain;
    }

private:
    void createSemaphores() {
        vk::SemaphoreCreateInfo info = {};

        m_acquireSemaphore = std::make_unique<vk::Semaphore>(m_queue->device(), info);
        m_renderSemaphore = std::make_unique<vk::Semaphore>(m_queue->device(), info);

        addExternalWait(*m_acquireSemaphore, vk::PipelineStageFlags::ColorAttachmentOutput);
        addExternalSignal(*m_renderSemaphore);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo info = {};
        info.queueFamilyIndex = m_family;
        info.flags = vk::CommandPoolCreateFlags::ResetCommandBuffer;

        m_commandPool = std::make_unique<vk::CommandPool>(m_queue->device(), info);

        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = m_commandPool.get();
        allocInfo.commandBufferCount = 1;

        m_commandBuffer = std::make_unique<vk::CommandBuffer>(std::move(m_commandPool->allocate(allocInfo)[0]));
    }

    void preRender() override {
        m_index = m_swapchain->acquireNextImage(~0, m_acquireSemaphore.get(), nullptr);
    }

    std::vector<const vk::CommandBuffer*> render() override {
        vk::CommandBuffer& commandBuffer = *m_commandBuffer;
        commandBuffer.reset(vk::CommandBufferResetFlags::None);

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlags::OneTimeSubmit;

        commandBuffer.begin(beginInfo);

        vk::ImageMemoryBarrier barrier = {};
        barrier.image = &m_swapchain->images()[m_index];
        barrier.oldLayout = vk::ImageLayout::Undefined;
        barrier.newLayout = vk::ImageLayout::PresentSrcKhr;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = vk::AccessFlags::None;
        barrier.dstAccessMask = vk::AccessFlags::None;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlags::Color;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlags::ColorAttachmentOutput, vk::PipelineStageFlags::BottomOfPipe, vk::DependencyFlags::None,
            {}, {}, { barrier });

        commandBuffer.end();

        return { &commandBuffer };
    }

    void postRender() override {
        vk::PresentInfo info = {};
        info.swapchains = { *m_swapchain };
        info.imageIndices = { m_index };
        info.waitSemaphores = { *m_renderSemaphore };

        m_queue->present(info);
    }

    vk::Swapchain* m_swapchain;
    std::unique_ptr<vk::Semaphore> m_acquireSemaphore;
    std::unique_ptr<vk::Semaphore> m_renderSemaphore;
    uint32_t m_index;
    std::unique_ptr<vk::CommandPool> m_commandPool;
    std::unique_ptr<vk::CommandBuffer> m_commandBuffer;
};

int main() {
    glfwInit();

    {
        Nova::Renderer renderer = Nova::Renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });
        Nova::Engine engine = Nova::Engine(renderer);

        for (auto& physicalDevice : renderer.validDevices()) {
            renderer.createDevice(*physicalDevice, {}, nullptr);
            break;
        }

        Nova::Window window = Nova::Window(engine, 800, 600);
        Nova::RenderGraph graph = Nova::RenderGraph(engine, window.swapchain().images().size());
        auto& node = graph.addNode(TestNode(*renderer.graphicsQueue(), window.swapchain()));
        graph.bake();

        auto slot1 = window.onSwapchainChanged().connectMember(node, &TestNode::setSwapchain);
        auto slot2 = window.onSwapchainChanged().connect([&](vk::Swapchain& swapchain) {
            graph.setFrames(swapchain.images().size());
        });

        while (!window.shouldClose()) {
            glfwPollEvents();
            window.update();
            graph.submit();
        }
    }

    glfwTerminate();
    return 0;
}