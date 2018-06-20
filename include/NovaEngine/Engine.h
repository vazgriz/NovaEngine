#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <NovaEngine/NovaEngine.h>

namespace Nova {
    class Engine {
    public:
        Engine(Renderer&& renderer);
        Engine(const Engine& other) = delete;
        Engine& operator = (const Engine& other) = delete;
        Engine(Engine&& other);
        Engine& operator = (Engine&& other) = default;

    private:
        Renderer renderer;
    };
}