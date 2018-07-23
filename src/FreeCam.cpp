#include "NovaEngine/FreeCam.h"
#include <algorithm>
#include <cmath>

using namespace Nova;

FreeCam::FreeCam(Window& window, Camera& camera, float sensitivity) {
    m_window = &window;
    m_input = &m_window->input();
    m_camera = &camera;
    m_sensitivity = sensitivity;

    m_look = {};
    m_position = { 0, 0, 1 };
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
        glm::quat rotation = glm::angleAxis(glm::radians(-m_look.x), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::radians(m_look.y), glm::vec3(1, 0, 0));
        glm::vec3 forward = rotation * glm::vec3(0, 0, -1);
        glm::vec3 right = rotation * glm::vec3(1, 0, 0);

        if (m_input->getButtonHold(Button::W)) {
            m_position += forward * delta;
        }
        
        if (m_input->getButtonHold(Button::S)) {
            m_position -= forward * delta;
        }
        
        if (m_input->getButtonHold(Button::D)) {
            m_position += right * delta;
        }
        
        if (m_input->getButtonHold(Button::A)) {
            m_position -= right * delta;
        }

        m_camera->setPosition(m_position);
        m_camera->setRotation(rotation);
    } else {
        if (m_input->getButtonDown(Button::Mouse1)) {
            m_locked = true;
            m_window->setMouseLocked(true);
        }
    }
}