#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <unordered_set>
#include <glm/glm.hpp>

namespace Nova {
    class Engine;
    class Window;

    enum class Button : int {
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        Num0 = 48,
        Num1 = 49,
        Num2 = 50,
        Num3 = 51,
        Num4 = 52,
        Num5 = 53,
        Num6 = 54,
        Num7 = 55,
        Num8 = 56,
        Num9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        KP0 = 320,
        KP1 = 321,
        KP2 = 322,
        KP3 = 323,
        KP4 = 324,
        KP5 = 325,
        KP6 = 326,
        KP7 = 327,
        KP8 = 328,
        KP9 = 329,
        KP_Decimal = 330,
        KP_Divide = 331,
        KP_Multiply = 332,
        KP_Subtract = 333,
        KP_Ad = 334,
        KP_Enter = 335,
        KP_Equal = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,
        Mouse1 = 400,
        Mouse2 = 401,
        Mouse3 = 402,
        Mouse4 = 403,
        Mouse5 = 404,
        Mouse6 = 405,
        Mouse7 = 406,
        Mouse8 = 407,
    };

    class Input {
        friend class Window;
        struct ButtonEvent {
            Button button;
            int action;
        };

    public:
        Input(Engine& engine, Window& window);
        Input(const Input& other) = delete;
        Input& operator = (const Input& other) = delete;
        Input(Input&& other) = default;
        Input& operator = (Input&& other) = default;

        bool getButtonDown(Button button) const;
        bool getButtonHold(Button button) const;
        bool getButtonUp(Button button) const;
        glm::vec2 getMousePos() const { return m_mousePos; }
        glm::vec2 getMouseDelta() const { return m_mouseDelta; }

    private:
        Engine* m_engine;
        Window* m_window;
        std::queue<ButtonEvent> m_buttonEvents;
        std::unordered_set<Button> m_buttonDown;
        std::unordered_set<Button> m_buttonUp;
        std::unordered_set<Button> m_buttonHold;
        glm::vec2 m_mousePos = {};
        glm::vec2 m_lastMousePos = {};
        glm::vec2 m_mouseDelta;

        void onKeyEvent(int key, int scancode, int action, int mods);
        void onMouseEvent(int mouseButton, int action, int mods);
        void onMouseMove(double xPos, double yPos);
        void update();
    };
}