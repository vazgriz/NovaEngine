#pragma once
#include <GLFW/glfw3.h>

namespace Nova {
    class Clock {
    public:
        void update();
        double actualTime() const;
        double frameTime() const { return m_last; }
        double deltaTime() const { return m_elapsed; }

    private:
        double m_last = 0;
        double m_elapsed;
    };
}