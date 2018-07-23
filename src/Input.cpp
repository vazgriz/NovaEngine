#include "NovaEngine/Input.h"
#include <GLFW/glfw3.h>

using namespace Nova;

Input::Input(Engine& engine, Window& window) {
    m_engine = &engine;
    m_window = &window;
}

void Input::onKeyEvent(int key, int scancode, int action, int mods) {
    Button button = static_cast<Button>(key);
    m_buttonEvents.push({ button, action });
}

void Input::onMouseEvent(int mouseButton, int action, int mods) {
    Button button = static_cast<Button>(mouseButton + static_cast<int>(Button::Mouse1));
    m_buttonEvents.push({ button, action });
}

void Input::onMouseMove(double xPos, double yPos) {
    m_mousePos = { static_cast<float>(xPos), static_cast<float>(yPos) };
}

void Input::update() {
    m_buttonDown.clear();
    m_buttonUp.clear();

    while (!m_buttonEvents.empty()) {
        auto event = m_buttonEvents.front();
        m_buttonEvents.pop();

        if (event.action == GLFW_PRESS) {
            m_buttonDown.insert(event.button);
            m_buttonHold.insert(event.button);

        } else if (event.action == GLFW_RELEASE) {
            m_buttonUp.insert(event.button);
            m_buttonHold.erase(event.button);
        }
    }

    m_mouseDelta = m_mousePos - m_lastMousePos;
    m_lastMousePos = m_mousePos;
}

bool Input::getButtonDown(Button button) const {
    return m_buttonDown.count(button) == 1;
}

bool Input::getButtonHold(Button button) const {
    return m_buttonHold.count(button) == 1;
}

bool Input::getButtonUp(Button button) const {
    return m_buttonUp.count(button) == 1;
}