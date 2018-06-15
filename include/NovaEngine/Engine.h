#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/NovaEngine.h>

namespace Nova {
    class Engine {
    public:
        Engine(Renderer&& renderer);
        Engine(const Renderer& other) = delete;
        Engine& operator = (const Renderer& other) = delete;
        Engine(Engine&& other);
        Engine& operator = (Engine&& other) = default;

    private:
        Renderer renderer;
    };
}