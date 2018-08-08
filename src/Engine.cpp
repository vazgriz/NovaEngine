#include <NovaEngine/Engine.h>

#define VIRTUAL_FRAMES 2

using namespace Nova;

Engine::Engine(Renderer& renderer) {
    m_renderer = &renderer;
    m_memory = std::make_unique<Memory>(*this);
    m_frameGraph = std::make_unique<FrameGraph>(*this);
    m_frameGraph->setFrameCount(VIRTUAL_FRAMES);
}

void Engine::addWindow(Window& window) {
    if (m_window != nullptr) {
        throw std::runtime_error("Window already set");
    }

    m_window = &window;
}

void Engine::addSystem(ISystem& system) {
    m_systems.push_back(&system);
}

void Engine::run() {
    while (!m_window->shouldClose()) {
        clock.update();

        glfwPollEvents();
        m_window->update();

        if (!m_window->canRender()) {
            glfwWaitEvents();
        } else {
            for (auto system : m_systems) {
                system->update(clock.deltaTime());
            }
            m_frameGraph->submit();
            m_memory->update(m_frameGraph->completedFrames());
        }
    }
}

void Engine::wait() {
    m_renderer->device().waitIdle();
}