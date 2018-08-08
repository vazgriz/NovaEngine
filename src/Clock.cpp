#include "NovaEngine/Clock.h"

using namespace Nova;

void Clock::update() {
    double now = glfwGetTime();
    m_elapsed = now - m_last;
    m_last = now;
}

double Clock::actualTime() const {
    return glfwGetTime();
}