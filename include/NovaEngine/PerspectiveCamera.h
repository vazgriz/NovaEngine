#pragma once
#include "NovaEngine/Camera.h"

namespace Nova {
    class PerspectiveCamera : public Camera {
    public:
        PerspectiveCamera(CameraManager& cameraManager, glm::ivec2 size, float fov, float near, float far);
        PerspectiveCamera(const PerspectiveCamera& other) = delete;
        PerspectiveCamera& operator = (const PerspectiveCamera& other) = delete;
        PerspectiveCamera(PerspectiveCamera&& other) = default;
        PerspectiveCamera& operator = (PerspectiveCamera&& other) = default;

        void setFOV(float fov);
        void setNearPlane(float near);
        void setFarPlane(float far);

    protected:
        glm::mat4 getProjection() override;

    private:
        float m_fov;
        float m_near;
        float m_far;
    };
}