#include "NovaEngine/Renderer.h"
#include "NovaEngine/Window.h"
#include <unordered_set>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace Nova;

const std::vector<std::string> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<std::string> merge(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::unordered_set<std::string> set;

    for (auto& str : a) {
        set.insert(str);
    }

    for (auto& str : b) {
        set.insert(str);
    }

    std::vector<std::string> result;

    for (auto& str : set) {
        result.emplace_back(std::move(str));
    }

    return result;
}

Renderer::Renderer(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers) {
    createInstance(appName, extensions, layers);
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

    uint32_t exCount;
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&exCount);
    std::vector<std::string> requiredExtensionsV;

    for (uint32_t i = 0; i < exCount; i++) {
        requiredExtensionsV.emplace_back(requiredExtensions[i]);
    }

    std::vector<std::string> extensionList = merge(requiredExtensionsV, extensions);

    vk::InstanceCreateInfo info = {};
    info.applicationInfo = &appInfo;
    info.enabledExtensionNames = std::move(extensionList);
    info.enabledLayerNames = layers;

    m_instance = std::make_unique<vk::Instance>(info);

    for (auto& physicalDevice : m_instance->physicalDevices()) {
        if (isValid(physicalDevice)) {
            m_validDevices.push_back(&physicalDevice);
        }
    }
}

void Renderer::createDevice(const vk::PhysicalDevice& physicalDevice, const std::vector<std::string>& extensions, vk::PhysicalDeviceFeatures* features) {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t transferFamily;
    bool graphicsFound = false;
    bool presentFound = false;
    bool transferFound = false;

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

        if (!transferFound && family.queueCount > 0
            && (family.queueFlags & vk::QueueFlags::Transfer) == vk::QueueFlags::Transfer
            && (family.queueFlags & vk::QueueFlags::Graphics) == vk::QueueFlags::None
            && (family.queueFlags & vk::QueueFlags::Compute) == vk::QueueFlags::None) {
            transferFound = true;
            transferFamily = i;
        }

        if (graphicsFound && presentFound && transferFound) break;
    }

    if (!transferFound) {
        transferFamily = graphicsFamily;
    }

    std::unordered_set<uint32_t> families = { graphicsFamily, presentFamily, transferFamily };
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

    std::vector<std::string> extensionList = merge(extensions, deviceExtensions);

    vk::DeviceCreateInfo info = {};
    info.queueCreateInfos = std::move(queueInfos);
    info.enabledFeatures = &features_;
    info.enabledExtensionNames = std::move(extensionList);

    m_device = std::make_unique<vk::Device>(physicalDevice, info);

    m_graphicsQueue = &m_device->getQueue(graphicsFamily, 0);
    m_presentQueue = &m_device->getQueue(presentFamily, 0);
    m_transferQueue = &m_device->getQueue(transferFamily, 0);
}