#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <VulkanWrapper/VulkanWrapper.h>

namespace Nova {
    class Engine;
    class Renderer;

    class Window {
    public:
        Window(Engine& engine, int32_t width, int32_t height, const std::string& title = "NovaEngine");
        Window(const Window& other) = delete;
        Window& operator = (const Window& other) = delete;
        Window(Window&& other) = default;
        Window& operator = (Window&& other) = default;

        Engine& engine() const { return *m_engine; }
        vk::Surface& surface() const { return *m_surface; }

        vk::Swapchain& swapchain() const { return *m_swapchain; }
        const std::vector<vk::ImageView>& imageViews() const { return m_imageViews; }

        bool shouldClose() const { return glfwWindowShouldClose(m_window.get()); }

    private:
        friend class Engine;
        Engine* m_engine;
        Renderer* m_renderer;
        const vk::PhysicalDevice* m_physicalDevice;
        std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> m_window;
        int32_t m_width;
        int32_t m_height;
        std::unique_ptr<vk::Surface> m_surface;
        std::unique_ptr<vk::Swapchain> m_swapchain;
        std::vector<vk::ImageView> m_imageViews;

        void createSurface();
        void recreateSwapchain();
        vk::SurfaceFormat chooseFormat(const std::vector<vk::SurfaceFormat>& formats);
        vk::PresentMode chooseMode(const std::vector<vk::PresentMode>& modes);
        vk::Extent2D chooseExtent(const vk::SurfaceCapabilities& capabilities);
        void createSwapchain();
        void createImageViews();
    };
}