#include "Camera.h"

#include <cassert>
#include <limits>

namespace Engine
{
    void Camera::setOrthographicProjection(float _left, float _right, float _bottom, float _top, float _near, float _far) {
        m_projectionMatrix = glm::mat4{ 1.0f };
        m_projectionMatrix[0][0] = 2.f / (_right - _left);
        m_projectionMatrix[1][1] = 2.f / (_bottom - _top);
        m_projectionMatrix[2][2] = 1.f / (_far - _near);
        m_projectionMatrix[3][0] = -(_right + _left) / (_right - _left);
        m_projectionMatrix[3][1] = -(_bottom + _top) / (_bottom - _top);
        m_projectionMatrix[3][2] = -_near / (_far - _near);
    }

    void Camera::setPerspectiveProjection(float _fovY, float _aspectRatio, float _near, float _far) {
        assert(glm::abs(_aspectRatio - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovY = tan(_fovY / 2.f);
        m_projectionMatrix = glm::mat4{ 0.0f };
        m_projectionMatrix[0][0] = 1.f / (_aspectRatio * tanHalfFovY);
        m_projectionMatrix[1][1] = 1.f / (tanHalfFovY);
        m_projectionMatrix[2][2] = _far / (_far - _near);
        m_projectionMatrix[2][3] = 1.f;
        m_projectionMatrix[3][2] = -(_far * _near) / (_far - _near);
    }
}