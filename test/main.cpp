#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <NovaEngine/NovaEngine.h>

int main() {
    glfwInit();

    Nova::Renderer renderer = Nova::Renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });
    Nova::Engine engine = Nova::Engine(std::move(renderer));

    glfwTerminate();
    return 0;
}