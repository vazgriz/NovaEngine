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
    }

    vk::Semaphore& acquireSemaphore() { return *m_acquireSemaphore; }
    vk::Semaphore& renderSemaphore() { return *m_renderSemaphore; }

private:
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

    vk::Swapchain* m_swapchain;
    std::unique_ptr<vk::Semaphore> m_acquireSemaphore;
    std::unique_ptr<vk::Semaphore> m_renderSemaphore;
    uint32_t m_index;
    std::unique_ptr<vk::CommandPool> m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<const vk::CommandBuffer*> m_commands;
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