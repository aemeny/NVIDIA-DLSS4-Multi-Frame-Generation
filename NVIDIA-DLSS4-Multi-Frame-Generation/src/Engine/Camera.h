#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Engine
{
    struct Camera 
    {
        void setOrthographicProjection(float _left, float _right, float _bottom, float _top, float _near, float _far);
        void setPerspectiveProjection(float _fovY, float _aspectRatio, float _near, float _far);

        void setViewDirection(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up = glm::vec3(0.0f, -1.0f, 0.0f));
        void setViewTarget(glm::vec3 _position, glm::vec3 _target, glm::vec3 _up = glm::vec3(0.0f, -1.0f, 0.0f));
        void setViewYXZ(glm::vec3 _position, glm::vec3 _rotation);

        const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
        const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
        const glm::mat4& getInverseViewMatrix() const { return m_inverseViewMatrix; }
        const glm::vec3 getPosition() const { return glm::vec3(m_inverseViewMatrix[3]); }

    private:
        glm::mat4 m_projectionMatrix{ 1.0f };
        glm::mat4 m_viewMatrix{ 1.0f };
        glm::mat4 m_inverseViewMatrix{ 1.0f };
    };
}