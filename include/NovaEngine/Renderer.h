#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <memory>
#include <vector>

namespace Nova {
    class Renderer {
    public:
        Renderer(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers);
        Renderer(const Renderer& other) = delete;
        Renderer& operator = (const Renderer& other) = delete;
        Renderer(Renderer&& other);
        Renderer& operator = (Renderer&& other) = default;

        vk::Instance& instance() { return *m_instance; }

    private:
        std::unique_ptr<vk::Instance> m_instance;

        void createInstance(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers);
    };
}