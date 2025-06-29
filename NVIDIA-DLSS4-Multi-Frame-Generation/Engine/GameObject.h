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
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
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