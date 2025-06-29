#pragma once
#include "ModelHandler.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
    struct TransformComponent
    {
        glm::vec3 m_translation{};
        glm::vec3 m_scale{1.0f};
        glm::vec3 m_rotation{0.0f}; // Rotation in radians

        // Matrix corresponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-Bryan angles of Y(1), X(2), Z(3)
        // https://pastebin.com/KFvG09A8
        glm::mat4 mat4() {
            const float c3 = glm::cos(m_rotation.z);
            const float s3 = glm::sin(m_rotation.z);
            const float c2 = glm::cos(m_rotation.x);
            const float s2 = glm::sin(m_rotation.x);
            const float c1 = glm::cos(m_rotation.y);
            const float s1 = glm::sin(m_rotation.y);
            return glm::mat4{
                {
                    m_scale.x * (c1 * c3 + s1 * s2 * s3),
                    m_scale.x * (c2 * s3),
                    m_scale.x * (c1 * s2 * s3 - c3 * s1),
                    0.0f,
                },
                {
                    m_scale.y * (c3 * s1 * s2 - c1 * s3),
                    m_scale.y * (c2 * c3),
                    m_scale.y * (c1 * c3 * s2 + s1 * s3),
                    0.0f,
                },
                {
                    m_scale.z * (c2 * s1),
                    m_scale.z * (-s2),
                    m_scale.z * (c1 * c2),
                    0.0f,
                },
                {m_translation.x, m_translation.y, m_translation.z, 1.0f} };
        }
    };

    struct GameObject 
    {
        using id_t = unsigned int;

        static GameObject createGameObject()
        {
            static id_t currentId = 0;
            return GameObject(currentId++);
        }
        id_t getId() const { return m_id; }

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject& operator=(GameObject&&) = default;

        std::shared_ptr<Model> m_model;
        glm::vec3 m_colour;
        TransformComponent m_transform;

    private:
        GameObject(id_t _objId) : m_id(_objId) {}
        id_t m_id;
    };
}