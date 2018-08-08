#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Renderer.h"
#include "NovaEngine/Window.h"
#include "NovaEngine/Memory.h"
#include "NovaEngine/FrameGraph.h"
#include "NovaEngine/ISystem.h"
#include "NovaEngine/Clock.h"

namespace Nova {
    class Engine {
    public:
        Engine(Renderer& m_renderer);
        Engine(const Engine& other) = delete;
        Engine& operator = (const Engine& other) = delete;
        Engine(Engine&& other) = default;
        Engine& operator = (Engine&& other) = default;

        Renderer& renderer() { return *m_renderer; }
        Memory& memory() { return *m_memory; }
        Window& window() { return *m_window; }
        FrameGraph& frameGraph() { return *m_frameGraph; }

        void addSystem(ISystem& system);
        void run();
        void wait();

    private:
        friend class Window;
        Renderer* m_renderer = nullptr;
        Window* m_window = nullptr;
        std::unique_ptr<Memory> m_memory;
        std::unique_ptr<FrameGraph> m_frameGraph;
        std::vector<ISystem*> m_systems;
        Clock clock;

        void addWindow(Window& window);
    };
}