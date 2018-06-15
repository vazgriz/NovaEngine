#include <NovaEngine/Engine.h>

using namespace Nova;

Engine::Engine(Renderer&& renderer) : renderer(std::move(renderer)) {

}

Engine::Engine(Engine&& other) : renderer(std::move(other.renderer)) {
    *this = std::move(other);
}