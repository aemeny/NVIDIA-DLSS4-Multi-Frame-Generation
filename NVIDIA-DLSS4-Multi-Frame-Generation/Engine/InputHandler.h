#pragma once

#include "GameObject.h"
#include "Window.h"

namespace Engine
{
    struct InputHandler 
    {
        struct keyMappings
        {
            static constexpr int MOVE_FORWARD = GLFW_KEY_W;
            static constexpr int MOVE_BACKWARD = GLFW_KEY_S;
            static constexpr int MOVE_LEFT = GLFW_KEY_A;
            static constexpr int MOVE_RIGHT = GLFW_KEY_D;
            static constexpr int MOVE_UP = GLFW_KEY_E;
            static constexpr int MOVE_DOWN = GLFW_KEY_Q;
            static constexpr int LOOK_LEFT = GLFW_KEY_LEFT;
            static constexpr int LOOK_RIGHT = GLFW_KEY_RIGHT;
            static constexpr int LOOK_UP = GLFW_KEY_UP;
            static constexpr int LOOK_DOWN = GLFW_KEY_DOWN;
        };

        keyMappings m_keys;
        float m_moveSpeed = 3.0f;
        float m_lookSpeed = 1.5f;

        void moveInPlaneXZ(GLFWwindow* _window, float _DT, GameObject& _gameObject);
    };
}