#include "NovaEngine/Renderer.h"
#include "NovaEngine/Window.h"
#include <unordered_set>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace Nova;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

Renderer::Renderer(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers) {
    createInstance(appName, extensions, layers);
}

Renderer::Renderer(Renderer&& other) {
    *this = std::move(other);
}

bool Renderer::isValid(const vk::PhysicalDevice& physicalDevice) {
    bool graphicsFound = false;
    bool presentFound = false;

    for (uint32_t i = 0; i < physicalDevice.queueFamilies().size(); i++) {
        const auto& family = physicalDevice.queueFamilies()[i];

        if (family.queueCount > 0
            && (family.queueFlags & vk::QueueFlags::Graphics) != vk::QueueFlags::None) {
            graphicsFound = true;
        }

        if (family.queueCount > 0
            && glfwGetPhysicalDevicePresentationSupport(physicalDevice.instance().handle(), physicalDevice.handle(), i)) {
            presentFound = true;
        }
    }

    return graphicsFound && presentFound;
}

void Renderer::createInstance(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers) {
    vk::ApplicationInfo appInfo = {};
    appInfo.applicationName = appName;
    appInfo.engineName = "NovaEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    std::unordered_set<std::string> extensionSet;
    for (auto& extension : extensions) {
        extensionSet.insert(extension);
    }

    uint32_t exCount;
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&exCount);

    for (uint32_t i = 0; i < exCount; i++) {
        extensionSet.insert(requiredExtensions[i]);
    }

    vk::InstanceCreateInfo info = {};
    info.applicationInfo = &appInfo;

    std::vector<std::string> extensionList;
    for (auto& extension : extensionSet) {
        info.enabledExtensionNames.emplace_back(std::move(extension));
    }

    info.enabledLayerNames = layers;

    m_instance = std::make_unique<vk::Instance>(info);
}

void Renderer::createDevice(const vk::PhysicalDevice& physicalDevice, const std::vector<std::string>& extensions, vk::PhysicalDeviceFeatures* features) {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool graphicsFound = false;
    bool presentFound = false;

    for (uint32_t i = 0; i < physicalDevice.queueFamilies().size(); i++) {
        auto& family = physicalDevice.queueFamilies()[i];

        if (!graphicsFound && family.queueCount > 0 && (family.queueFlags & vk::QueueFlags::Graphics) != vk::QueueFlags::None) {
            graphicsFamily = i;
            graphicsFound = true;
        }

        if (!presentFound && family.queueCount > 0
            && glfwGetPhysicalDevicePresentationSupport(m_instance->handle(), physicalDevice.handle(), i)) {
            presentFamily = i;
            presentFound = true;
        }

        if (graphicsFound && presentFound) break;
    }

    std::unordered_set<uint32_t> families = { graphicsFamily, presentFamily };
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;
    float priority = 1;

    for (auto family : families) {
        vk::DeviceQueueCreateInfo queueInfo = {};
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.queuePriorities = { priority };
        queueInfos.push_back(queueInfo);
    }

    vk::PhysicalDeviceFeatures features_ = {};
    if (features != nullptr) {
        features_ = *features;
    }

    std::unordered_set<std::string> extensionSet;
    for (auto& ex : extensions) {
        extensionSet.insert(ex);
    }

    for (auto& ex : deviceExtensions) {
        extensionSet.insert(ex);
    }

    std::vector<std::string> extensionList;
    for (auto& ex : extensionSet) {
        extensionList.emplace_back(std::move(ex));
    }

    vk::DeviceCreateInfo info = {};
    info.queueCreateInfos = std::move(queueInfos);
    info.enabledFeatures = &features_;
    info.enabledExtensionNames = std::move(extensionList);

    m_device = std::make_unique<vk::Device>(physicalDevice, info);

    m_graphicsQueue = &m_device->getQueue(graphicsFamily, 0);
    m_presentQueue = &m_device->getQueue(presentFamily, 0);
}