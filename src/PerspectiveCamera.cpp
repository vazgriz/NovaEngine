#include "NovaEngine/PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp> 

using namespace Nova;

const glm::mat4 correctionMatrix = {
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, -0.5f, 0,
    0, 0, 0.5f, 1
};

PerspectiveCamera::PerspectiveCamera(Engine& engine, CameraManager& cameraManager, BufferAllocator& allocator, glm::ivec2 size, float fov)
    : Camera(engine, cameraManager, allocator, size) {
    m_fov = fov;
}

glm::mat4 PerspectiveCamera::getProjection() {
    return correctionMatrix * glm::perspectiveFovRH_ZO(m_fov, static_cast<float>(size().x), static_cast<float>(size().y), 0.1f, 10.0f);
}