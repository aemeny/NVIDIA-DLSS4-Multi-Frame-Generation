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

        glm::mat4 getProjectionMatrix() const { return m_projectionMatrix; }

    private:
        glm::mat4 m_projectionMatrix{ 1.0f };
    };
}