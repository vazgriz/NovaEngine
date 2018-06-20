#include <NovaEngine/Engine.h>

using namespace Nova;

Engine::Engine(Renderer&& m_renderer) : m_renderer(std::move(m_renderer)) {

}

Engine::Engine(Engine&& other) : m_renderer(std::move(other.m_renderer)) {
    *this = std::move(other);
}