#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Renderer.h"
#include "NovaEngine/Window.h"
#include "NovaEngine/Memory.h"
#include "NovaEngine/QueueGraph.h"

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
        QueueGraph& queueGraph() { return *m_queueGraph; }

    private:
        friend class Window;
        Renderer* m_renderer = nullptr;
        Window* m_window = nullptr;
        std::unique_ptr<Memory> m_memory;
        std::unique_ptr<QueueGraph> m_queueGraph;

        void addWindow(Window& window);
    };
}