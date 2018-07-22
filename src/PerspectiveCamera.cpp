#include "NovaEngine/PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp> 

using namespace Nova;

PerspectiveCamera::PerspectiveCamera(Engine& engine, BufferAllocator& allocator, glm::ivec2 size, float fov) : Camera(engine, allocator, size) {
    m_fov = fov;
}

glm::mat4 PerspectiveCamera::getProjection() {
    return glm::perspectiveFovRH_ZO(m_fov, static_cast<float>(size().x), static_cast<float>(size().y), 0.1f, 10.0f);
}