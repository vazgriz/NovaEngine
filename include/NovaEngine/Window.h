#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Engine.h"

namespace Nova {
    class Window {
    public:
        Window(Engine& engine, int32_t width, int32_t height, const std::string& title = "NovaEngine");
        Window(const Window& other) = delete;
        Window& operator = (const Window& other) = delete;
        Window(Window&& other);
        Window& operator = (Window&& other);
        ~Window();

        Engine& engine() const { return *m_engine; }
        vk::Surface& surface() const { return *m_surface; }

    private:
        Engine* m_engine;
        GLFWwindow* m_window;
        std::unique_ptr<vk::Surface> m_surface;

        void createSurface();
    };
}