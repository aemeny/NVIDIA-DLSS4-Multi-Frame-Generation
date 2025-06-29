#include "InputHandler.h"

#include <limits>

namespace Engine
{
    void InputHandler::moveInPlaneXZ(GLFWwindow* _window, float _DT, GameObject& _gameObject)
    {
        glm::vec3 rotate(0.0f);

        if (glfwGetKey(_window, m_keys.LOOK_RIGHT) == GLFW_PRESS) rotate.y += 1.0f;
        if (glfwGetKey(_window, m_keys.LOOK_LEFT) == GLFW_PRESS) rotate.y -= 1.0f;
        if (glfwGetKey(_window, m_keys.LOOK_UP) == GLFW_PRESS) rotate.x += 1.0f;
        if (glfwGetKey(_window, m_keys.LOOK_DOWN) == GLFW_PRESS) rotate.x -= 1.0f;

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
            _gameObject.m_transform.m_rotation += m_lookSpeed * _DT * glm::normalize(rotate);

        _gameObject.m_transform.m_rotation.x = glm::clamp(_gameObject.m_transform.m_rotation.x, -glm::half_pi<float>(), glm::half_pi<float>());
        _gameObject.m_transform.m_rotation.y = glm::mod(_gameObject.m_transform.m_rotation.y, glm::two_pi<float>());


        float yaw = _gameObject.m_transform.m_rotation.y;
        const glm::vec3 forwardDirection(glm::sin(yaw), 0.0f, glm::cos(yaw));
        const glm::vec3 rightDirection(forwardDirection.z, 0.0f, -forwardDirection.x);
        const glm::vec3 upDirection(0.0f, -1.0f, 0.0f); // -1.0f because y axis points down in Vulkan

        glm::vec3 moveDirection(0.0f);

        if (glfwGetKey(_window, m_keys.MOVE_FORWARD) == GLFW_PRESS) moveDirection += forwardDirection;
        if (glfwGetKey(_window, m_keys.MOVE_BACKWARD) == GLFW_PRESS) moveDirection -= forwardDirection;
        if (glfwGetKey(_window, m_keys.MOVE_RIGHT) == GLFW_PRESS) moveDirection += rightDirection;
        if (glfwGetKey(_window, m_keys.MOVE_LEFT) == GLFW_PRESS) moveDirection -= rightDirection;
        if (glfwGetKey(_window, m_keys.MOVE_UP) == GLFW_PRESS) moveDirection += upDirection;
        if (glfwGetKey(_window, m_keys.MOVE_DOWN) == GLFW_PRESS) moveDirection -= upDirection;

        if (glm::dot(moveDirection, moveDirection) > std::numeric_limits<float>::epsilon())
            _gameObject.m_transform.m_translation += m_moveSpeed * _DT * glm::normalize(moveDirection);
    }
}