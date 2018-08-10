#include "NovaEngine/FreeCam.h"
#include <algorithm>
#include <cmath>
#include "NovaEngine/Window.h"
#include "NovaEngine/Input.h"
#include "NovaEngine/Camera.h"

using namespace Nova;

FreeCam::FreeCam(Window& window, Camera& camera, float sensitivity, float speed) {
    m_window = &window;
    m_input = &m_window->input();
    m_camera = &camera;
    m_sensitivity = sensitivity;
    m_speed = speed;

    m_look = {};
    m_position = m_camera->position();
}

void FreeCam::update(float delta) {
    if (m_locked) {
        if (m_input->getButtonDown(Button::Escape)) {
            m_locked = false;
            m_window->setMouseLocked(false);
            return;
        }

        m_look += m_input->getMouseDelta() * m_sensitivity;
        m_look.y = std::min(std::max(-90.0f, m_look.y), 90.0f);
        m_look.x = fmod(m_look.x + 360.0f, 360.0f);
        glm::quat rotation = glm::angleAxis(glm::radians(-m_look.x), glm::vec3(0, 1, 0)) * glm::angleAxis(-glm::radians(m_look.y), glm::vec3(1, 0, 0));
        glm::vec3 forward = rotation * glm::vec3(0, 0, -1);
        glm::vec3 right = rotation * glm::vec3(1, 0, 0);
        glm::vec3 up = rotation * glm::vec3(0, 1, 0);

        glm::vec3 move = {};

        if (m_input->getButtonHold(Button::W)) {
            move += forward;
        }
        
        if (m_input->getButtonHold(Button::S)) {
            move -= forward;
        }
        
        if (m_input->getButtonHold(Button::D)) {
            move += right;
        }
        
        if (m_input->getButtonHold(Button::A)) {
            move -= right;
        }

        if (m_input->getButtonHold(Button::E)) {
            move += up;
        }

        if (m_input->getButtonHold(Button::Q)) {
            move -= up;
        }

        if (glm::distance(move, glm::vec3{}) > 0.1f) {
            move = glm::normalize(move) * delta * m_speed;
        }
        m_position += move;

        m_camera->setPosition(m_position);
        m_camera->setRotation(rotation);
    } else {
        if (m_input->getButtonDown(Button::Mouse1)) {
            m_locked = true;
            m_window->setMouseLocked(true);
        }
    }
}