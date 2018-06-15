#include <iostream>
#include <NovaEngine/NovaEngine.h>

int main() {
    Nova::Renderer renderer("Test", {}, { "VK_LAYER_LUNARG_standard_validation" });
    Nova::Engine engine(std::move(renderer));
    return 0;
}