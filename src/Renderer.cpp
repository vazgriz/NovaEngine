#include <NovaEngine/Renderer.h>
#include <unordered_set>
#include <GLFW/glfw3.h>

using namespace Nova;

Renderer::Renderer(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers) {
    CreateInstance(appName, extensions, layers);
}

Renderer::Renderer(Renderer&& other) {
    *this = std::move(other);
}

void Renderer::CreateInstance(const std::string& appName, const std::vector<std::string>& extensions, const std::vector<std::string>& layers) {
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