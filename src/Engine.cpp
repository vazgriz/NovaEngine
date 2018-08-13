#include <NovaEngine/Engine.h>

#define VIRTUAL_FRAMES 2

using namespace Nova;

Engine::Engine(Renderer& renderer) {
    m_renderer = &renderer;
    m_memory = std::make_unique<Memory>(*this);
    m_frameGraph = std::make_unique<FrameGraph>(*this, VIRTUAL_FRAMES);
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

void Engine::step() {
    m_clock.update();
    m_memory->update(m_frameGraph->completedFrames());

    glfwPollEvents();
    m_window->update();

    if (!m_window->canRender()) {
        glfwWaitEvents();
    } else {
        for (auto system : m_systems) {
            system->update(static_cast<float>(m_clock.deltaTime()));
        }
        m_frameGraph->submit();
    }
}

void Engine::wait() {
    m_renderer->device().waitIdle();
}