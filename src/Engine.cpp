#include <NovaEngine/Engine.h>

using namespace Nova;

Engine::Engine(Renderer& renderer) {
    m_renderer = &renderer;
}

Engine::Engine(Engine&& other) {
    *this = std::move(other);
}

void Engine::addWindow(Window& window) {
    m_windows.push_back(&window);
}