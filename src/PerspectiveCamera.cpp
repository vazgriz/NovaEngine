#include "NovaEngine/PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using namespace Nova;

PerspectiveCamera::PerspectiveCamera(Engine& engine, CameraManager& cameraManager, BufferAllocator& allocator, glm::ivec2 size, float fov)
    : Camera(engine, cameraManager, allocator, size) {
    m_fov = fov;
}

glm::mat4 PerspectiveCamera::getProjection() {
    //http://dev.theomader.com/depth-precision/
    float f = 1.0f / tan(glm::radians(m_fov) / 2.0f);
    float aspect = size().x / static_cast<float>(size().y);
    glm::mat4 projection = {
        f / aspect, 0, 0, 0,
        0, -f, 0, 0,
        0, 0, 0, -1,
        0, 0, 0.1f, 0
    };
    return correctionMatrix * projection;
}