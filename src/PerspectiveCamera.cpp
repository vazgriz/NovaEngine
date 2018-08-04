#include "NovaEngine/PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using namespace Nova;

PerspectiveCamera::PerspectiveCamera(Engine& engine, CameraManager& cameraManager, BufferAllocator& allocator, glm::ivec2 size, float fov, float near, float far)
    : Camera(engine, cameraManager, allocator, size) {
    m_fov = fov;
    m_near = near;
    m_far = far;
}

void PerspectiveCamera::setFOV(float fov) {
    m_fov = fov;
}

void PerspectiveCamera::setNearPlane(float near) {
    m_near = near;
}

void PerspectiveCamera::setFarPlane(float far) {
    m_far = far;
}

glm::mat4 PerspectiveCamera::getProjection() {
    //http://dev.theomader.com/depth-precision/
    float f = 1.0f / tan(glm::radians(m_fov) / 2.0f);
    float aspect = size().x / static_cast<float>(size().y);
    glm::mat4 projection;

    if (m_far == std::numeric_limits<float>::infinity()) {
        projection = {
            f / aspect, 0, 0, 0,
            0, -f, 0, 0,
            0, 0, 0, -1,
            0, 0, m_near, 0
        };
    } else {
        projection = {
            f / aspect, 0, 0, 0,
            0, -f, 0, 0,
            0, 0, -(m_far / (m_near - m_far)) - 1.0f, -1.0f,
            0, 0, -((m_near * m_far) / (m_near - m_far)), 0
        };
    }

    return projection;
}