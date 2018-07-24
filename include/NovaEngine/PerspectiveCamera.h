#pragma once
#include "NovaEngine/Camera.h"

namespace Nova {
    class PerspectiveCamera : public Camera {
    public:
        PerspectiveCamera(Engine& engine, CameraManager& cameraManager, BufferAllocator& allocator, glm::ivec2 size, float fov);
        PerspectiveCamera(const PerspectiveCamera& other) = delete;
        PerspectiveCamera& operator = (const PerspectiveCamera& other) = delete;
        PerspectiveCamera(PerspectiveCamera&& other) = default;
        PerspectiveCamera& operator = (PerspectiveCamera&& other) = default;

    protected:
        glm::mat4 getProjection() override;

    private:
        float m_fov;
    };
}