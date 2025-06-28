#pragma once
#include "ModelHandler.h"

namespace Engine
{
    struct Transform2DComponent
    {
        glm::vec2 m_translation{};
        glm::vec2 m_scale{ 1.0f, 1.0f };
        float m_rotation = 0.0f; // Rotation in radians

        glm::mat2 mat2() 
        { 
            const float s = std::sin(m_rotation);
            const float c = std::cos(m_rotation);
            glm::mat2 rotationMat = { { c, s },
                                      { -s, c } };

            glm::mat2 scaleMat = { { m_scale.x, 0.0f },
                                   { 0.0f, m_scale.y } };

            return rotationMat * scaleMat;
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
        Transform2DComponent m_transform2D;

    private:
        GameObject(id_t _objId) : m_id(_objId) {}
        id_t m_id;
    };
}