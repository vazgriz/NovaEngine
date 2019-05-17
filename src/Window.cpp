#include "NovaEngine/Window.h"
#include "NovaEngine/Engine.h"

using namespace Nova;

Window::Window(Engine& engine, glm::ivec2 size, const std::string& title) : m_window(nullptr, nullptr) {
    m_engine = &engine;
    m_renderer = &m_engine->renderer();
    m_size = size;
    m_physicalDevice = &m_renderer->device().physicalDevice();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, 0);
    m_window = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)>(
        glfwCreateWindow(m_size.x, m_size.y, title.size() > 0 ? title.c_str() : nullptr, nullptr, nullptr),
        glfwDestroyWindow
    );

    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetWindowSizeCallback(m_window.get(), &Window::onResize);
    glfwSetWindowIconifyCallback(m_window.get(), &Window::onIconify);
    glfwSetKeyCallback(m_window.get(), &Window::onKeyEvent);
    glfwSetMouseButtonCallback(m_window.get(), &Window::onMouseEvent);
    glfwSetCursorPosCallback(m_window.get(), &Window::onMouseMove);

    m_zeroSized = (m_size.x == 0 || m_size.y == 0);

    createSurface();
    recreateSwapchain();
    createInput();

    engine.addWindow(*this);
}

void Window::onResize(GLFWwindow* window, int width, int height) {
    reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->onResize(static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void Window::onResize(int32_t width, int32_t height) {
    m_resized = true;
}

void Window::onIconify(GLFWwindow* window, int iconified) {
    reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->onIconify(iconified);
}

void Window::onIconify(bool iconified) {
    m_iconified = iconified;
}

void Window::onKeyEvent(GLFWwindow* window, int button, int scancode, int action, int mods) {
    reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->m_input->onKeyEvent(button, scancode, action, mods);
}

void Window::onMouseEvent(GLFWwindow* window, int button, int action, int mods) {
    reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->m_input->onMouseEvent(button, action, mods);
}

void Window::onMouseMove(GLFWwindow* window, double xPos, double yPos) {
    reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->m_input->onMouseMove(xPos, yPos);
}

bool Window::canRender() const {
    return !m_iconified && !m_zeroSized;
}

void Window::update() {
    if (m_resized) {
        m_resized = false;

        glm::ivec2 newSize;
        glfwGetFramebufferSize(m_window.get(), &newSize.x, &newSize.y);

        if (newSize == m_size) {
            return;
        }

        if (newSize.x == 0 || newSize.y == 0) {
            m_zeroSized = true;
            return;
        } else {
            m_zeroSized = false;
        }

        m_size = newSize;

        recreateSwapchain();

        m_swapchainSignal(*m_swapchain);
        m_resizeSignal(m_size);
    }

    m_input->update();
}

void Window::createSurface() {
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(m_engine->renderer().instance().handle(), m_window.get(), nullptr, &surface);
    m_surface = std::make_unique<vk::Surface>(m_engine->renderer().instance(), surface);

    bool supported = m_surface->supported(*m_physicalDevice, m_renderer->presentQueue().familyIndex());
    if (!supported) {
        throw std::runtime_error("Surface is not supported on this Physical Device");
    }
}

void Window::recreateSwapchain() {
    m_engine->renderer().device().waitIdle();
    createSwapchain();
    createImageViews();
}

vk::SurfaceFormat Window::chooseFormat(const std::vector<vk::SurfaceFormat>& formats) {
    if (formats.size() == 1 && formats[0].format == vk::Format::Undefined) {
        return { vk::Format::B8G8R8A8_Unorm, vk::ColorSpace::SrgbNonlinear };
    }
    
    for (const auto& format : formats) {
        if (format.format == vk::Format::B8G8R8A8_Unorm && format.colorSpace == vk::ColorSpace::SrgbNonlinear) {
            return format;
        }
    }

    return formats[0];
}

vk::PresentMode Window::chooseMode(const std::vector<vk::PresentMode>& modes) {
    return vk::PresentMode::Fifo;
}

vk::Extent2D Window::chooseExtent(const vk::SurfaceCapabilities& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y) };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Window::createSwapchain() {
    auto formats = m_surface->getFormats(*m_physicalDevice);
    auto modes = m_surface->getPresentModes(*m_physicalDevice);
    auto capabilities = m_surface->getCapabilities(*m_physicalDevice);

    auto format = chooseFormat(formats);
    auto mode = chooseMode(modes);
    auto extent = chooseExtent(capabilities);
    
    uint32_t imageCount = 2;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfo info = {};
    info.surface = m_surface.get();
    info.minImageCount = imageCount;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.presentMode = mode;
    info.imageArrayLayers = 1;
    info.imageUsage = vk::ImageUsageFlags::ColorAttachment;

    std::vector<uint32_t> familyIndices = { m_renderer->graphicsQueue().familyIndex(), m_renderer->presentQueue().familyIndex() };
    if (familyIndices[0] != familyIndices[1]) {
        info.queueFamilyIndices = std::move(familyIndices);
        info.imageSharingMode = vk::SharingMode::Concurrent;
    }

    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = vk::CompositeAlphaFlags::Opaque;
    info.clipped = true;
    info.oldSwapchain = m_swapchain.get();

    m_swapchain = std::make_unique<vk::Swapchain>(m_renderer->device(), info);
}

void Window::createImageViews() {
    m_imageViews.reserve(m_swapchain->images().size());
    for (size_t i = 0; i < m_swapchain->images().size(); i++) {
        vk::ImageViewCreateInfo info = {};
        info.image = &m_swapchain->images()[i];
        info.format = m_swapchain->images()[i].format();
        info.viewType = vk::ImageViewType::_2D;
        info.subresourceRange.aspectMask = vk::ImageAspectFlags::Color;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;

        m_imageViews.emplace_back(m_renderer->device(), info);
    }
}
void Window::createInput() {
    m_input = std::make_unique<Input>(*m_engine, *this);
}

void Window::setVisible(bool visible) {
    if (visible) {
        glfwShowWindow(m_window.get());
    } else {
        glfwHideWindow(m_window.get());
    }
}

bool Window::visible() const {
    int visible = glfwGetWindowAttrib(m_window.get(), GLFW_VISIBLE);
    return visible == 1;
}

void Window::setTitle(const std::string& title) {
    m_title = title;
    glfwSetWindowTitle(m_window.get(), m_title.c_str());
}

void Window::setMouseLocked(bool locked) {
    if (locked) {
        glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}