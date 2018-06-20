#include <iostream>
#include <NovaEngine/NovaEngine.h>

int main() {
    Nova::Renderer renderer = Nova::Renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });
    Nova::Engine engine = Nova::Engine(std::move(renderer));
    return 0;
}