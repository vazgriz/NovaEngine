#include "NovaEngine/Window.h"

using namespace Nova;

Window::Window(Engine& engine, int32_t width, int32_t height, const std::string& title) {
    m_engine = &engine;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.size() > 0 ? title.c_str() : nullptr, nullptr, nullptr);
    createSurface();
}

Window::Window(Window&& other) {
    *this = std::move(other);
}

Window& Window::operator = (Window&& other) {
    m_window = other.m_window;
    other.m_window = nullptr;
    m_engine = other.m_engine;
    m_surface = std::move(other.m_surface);
    return *this;
}

Window::~Window() {
    glfwDestroyWindow(m_window);
}

void Window::createSurface() {
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(m_engine->renderer().instance().handle(), m_window, nullptr, &surface);
    m_surface = std::make_unique<vk::Surface>(m_engine->renderer().instance(), surface);
}