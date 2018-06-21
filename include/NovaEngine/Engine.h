#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Renderer.h"

namespace Nova {
    class Engine {
    public:
        Engine(Renderer& m_renderer);
        Engine(const Engine& other) = delete;
        Engine& operator = (const Engine& other) = delete;
        Engine(Engine&& other);
        Engine& operator = (Engine&& other) = default;

        Renderer& renderer() { return *m_renderer; }

    private:
        Renderer* m_renderer;
    };
}