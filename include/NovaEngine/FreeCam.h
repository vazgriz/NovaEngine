#pragma once
#include <glm/glm.hpp>
#include "NovaEngine/ISystem.h"

namespace Nova {
    class Window;
    class Input;
    class Camera;

    class FreeCam : public ISystem {
    public:
        FreeCam(Window& window, Camera& camera, float sensitivity, float speed);
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
        float m_speed;
        glm::vec2 m_look;
        glm::vec3 m_position;
        bool m_locked = false;
    };
}