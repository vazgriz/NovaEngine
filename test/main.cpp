#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <NovaEngine/NovaEngine.h>

int main() {
    glfwInit();

    Nova::Renderer renderer = Nova::Renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });
    Nova::Engine engine = Nova::Engine(renderer);
    Nova::Window window = Nova::Window(engine, 800, 600);

    for (auto& physicalDevice : renderer.instance().physicalDevices()) {
        if (renderer.isValid(physicalDevice, window)) {
            renderer.createDevice(physicalDevice, window, {}, nullptr);
            break;
        }
    }

    engine.addWindow(window);

    glfwTerminate();
    return 0;
}