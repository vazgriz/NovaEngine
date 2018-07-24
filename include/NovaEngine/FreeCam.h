#pragma once
#include "NovaEngine/Window.h"
#include "NovaEngine/Camera.h"
#include "NovaEngine/ISystem.h"

namespace Nova {
    class FreeCam : public ISystem {
    public:
        FreeCam(Window& window, Camera& camera, float sensitivity);
        FreeCam(const FreeCam& other) = delete;
        FreeCam& operator = (const FreeCam& other) = delete;
        FreeCam(FreeCam&& other) = default;
        FreeCam& operator = (FreeCam&& other) = default;

        void update(float delta) override;

    private:
        Window* m_window;
        Input* m_input;
        Camera* m_camera;
        float m_sensitivity;
        glm::vec2 m_look;
        glm::vec3 m_position;
        bool m_locked = false;
    };
}