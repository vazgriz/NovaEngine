#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Signal.h"

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

        void update();
        Signal<vk::Swapchain&>& onSwapchainChanged() { return *m_swapchainSignal; }
        Signal<int32_t, int32_t>& onResize() { return *m_resizeSignal; }
        Signal<bool>& onIconify() { return *m_iconifySignal; }

        bool shouldClose() const { return glfwWindowShouldClose(m_window.get()); }
        bool iconified() const { return m_iconified; }
        bool canRender() const;

    private:
        friend class Engine;
        Engine* m_engine;
        Renderer* m_renderer;
        const vk::PhysicalDevice* m_physicalDevice;
        std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> m_window;
        int32_t m_width;
        int32_t m_height;
        bool m_zeroSized = false;
        bool m_iconified = false;
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

        bool m_resized = false;
        std::unique_ptr<Signal<vk::Swapchain&>> m_swapchainSignal;
        std::unique_ptr<Signal<int32_t, int32_t>> m_resizeSignal;
        std::unique_ptr<Signal<bool>> m_iconifySignal;

        static void onResize(GLFWwindow* window, int width, int height);
        void onResize(int32_t width, int32_t height);

        static void onIconify(GLFWwindow* window, int iconified);
        void onIconify(bool iconified);
    };
}