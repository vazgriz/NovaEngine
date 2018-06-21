#include <NovaEngine/Engine.h>

using namespace Nova;

Engine::Engine(Renderer& renderer) {
    m_renderer = &renderer;
}

Engine::Engine(Engine&& other) {
    *this = std::move(other);
}