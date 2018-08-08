#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Signal.h"
#include "NovaEngine/Input.h"

namespace Nova {
    class Engine;
    class Renderer;
    class Input;

    class Window {
    public:
        Window(Engine& engine, glm::ivec2 size, const std::string& title = "NovaEngine");
        Window(const Window& other) = delete;
        Window& operator = (const Window& other) = delete;
        Window(Window&& other) = default;
        Window& operator = (Window&& other) = default;

        Engine& engine() const { return *m_engine; }
        vk::Surface& surface() const { return *m_surface; }

        vk::Swapchain& swapchain() const { return *m_swapchain; }
        const std::vector<vk::ImageView>& imageViews() const { return m_imageViews; }
        glm::ivec2 size() const { return m_size; }

        Input& input() const { return *m_input; }

        void update();
        Signal<vk::Swapchain&>& onSwapchainChanged() { return *m_swapchainSignal; }
        Signal<glm::ivec2>& onResize() { return *m_resizeSignal; }
        Signal<bool>& onIconify() { return *m_iconifySignal; }

        bool shouldClose() const { return glfwWindowShouldClose(m_window.get()); }
        bool iconified() const { return m_iconified; }
        void setVisible(bool visible);
        bool visible() const;
        void setTitle(const std::string& title);
        std::string title() const { return m_title; }
        bool canRender() const;

        void setMouseLocked(bool locked);

    private:
        friend class Engine;
        Engine* m_engine;
        Renderer* m_renderer;
        const vk::PhysicalDevice* m_physicalDevice;
        std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> m_window;
        glm::ivec2 m_size;
        bool m_zeroSized = false;
        bool m_iconified = false;
        std::unique_ptr<vk::Surface> m_surface;
        std::unique_ptr<vk::Swapchain> m_swapchain;
        std::vector<vk::ImageView> m_imageViews;
        std::unique_ptr<Input> m_input;
        std::string m_title;

        void createSurface();
        void recreateSwapchain();
        vk::SurfaceFormat chooseFormat(const std::vector<vk::SurfaceFormat>& formats);
        vk::PresentMode chooseMode(const std::vector<vk::PresentMode>& modes);
        vk::Extent2D chooseExtent(const vk::SurfaceCapabilities& capabilities);
        void createSwapchain();
        void createImageViews();
        void createInput();

        bool m_resized = false;
        std::unique_ptr<Signal<vk::Swapchain&>> m_swapchainSignal;
        std::unique_ptr<Signal<glm::ivec2>> m_resizeSignal;
        std::unique_ptr<Signal<bool>> m_iconifySignal;

        static void onResize(GLFWwindow* window, int width, int height);
        void onResize(int32_t width, int32_t height);

        static void onIconify(GLFWwindow* window, int iconified);
        void onIconify(bool iconified);

        static void onKeyEvent(GLFWwindow* window, int button, int scancode, int action, int mods);
        static void onMouseEvent(GLFWwindow* window, int button, int action, int mods);
        static void onMouseMove(GLFWwindow* window, double xPos, double yPos);
    };
}