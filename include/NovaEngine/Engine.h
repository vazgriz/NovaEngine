#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Renderer.h"
#include "NovaEngine/Window.h"

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
        friend class Window;
        Renderer* m_renderer;
        std::vector<Window*> m_windows;

        void addWindow(Window& window);
    };
}