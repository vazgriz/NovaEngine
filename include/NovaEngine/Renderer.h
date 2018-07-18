#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <memory>
#include <vector>

namespace Nova {
    class Window;

    class Renderer {
    public:
        Renderer(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers);
        Renderer(const Renderer& other) = delete;
        Renderer& operator = (const Renderer& other) = delete;
        Renderer(Renderer&& other) = default;
        Renderer& operator = (Renderer&& other) = default;

        static bool isValid(const vk::PhysicalDevice& physicalDevice);

        vk::Instance& instance() { return *m_instance; }
        std::vector<const vk::PhysicalDevice*> validDevices() { return m_validDevices; }
        vk::Device& device() { return *m_device; }

        const vk::Queue& graphicsQueue() const { return *m_graphicsQueue; }
        const vk::Queue& presentQueue() const { return *m_presentQueue; }
        const vk::Queue& transferQueue() const { return *m_transferQueue; }

        void createDevice(const vk::PhysicalDevice& physicalDevice, const std::vector<std::string>& extensions, vk::PhysicalDeviceFeatures* features);

    private:
        std::unique_ptr<vk::Instance> m_instance;
        std::vector<const vk::PhysicalDevice*> m_validDevices;
        std::unique_ptr<vk::Device> m_device;
        const vk::Queue* m_graphicsQueue;
        const vk::Queue* m_presentQueue;
        const vk::Queue* m_transferQueue;

        void createInstance(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers);
    };
}