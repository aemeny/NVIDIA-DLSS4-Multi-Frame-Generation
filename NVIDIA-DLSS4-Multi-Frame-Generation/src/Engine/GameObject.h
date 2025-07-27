#pragma once
#include "ModelHandler.h"
#include "texture.h"

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

    struct PointLightComponent
    {
        float m_intensity = 1.0f;
    };

    struct GameObject 
    {
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, GameObject>;

        static GameObject createGameObject()
        {
            static id_t currentId = 0;
            return GameObject(currentId++);
        }
        id_t getId() const { return m_id; }

        static GameObject makePointLight(float _intensity = 5.0f, float _radius = 0.1f, glm::vec3 _colour = glm::vec3(1.0f));

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject& operator=(GameObject&&) = default;

        glm::vec3 m_colour;
        TransformComponent m_transform;

        // Optional components
        std::shared_ptr<Model> m_model;
        std::shared_ptr<Texture> m_diffuseMap = nullptr;
        std::unique_ptr<PointLightComponent> m_pointLight = nullptr;

    private:
        GameObject(id_t _objId) : m_id(_objId) {}
        id_t m_id;
    };
}