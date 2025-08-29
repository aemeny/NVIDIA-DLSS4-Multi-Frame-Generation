#pragma once
#include "GameObject.h"

namespace Engine
{
    struct SceneTester
    {
        enum class SceneType 
        {
            StaticGrid = 0,
            CameraPan = 1,
            MovingScene = 2,
            TrasnsparencyTest = 3
        };

        struct SceneLoader
        {
            SceneLoader(EngineDevice& device) : m_device(device) {}

            // Static, GPU-heavy grid of a shared model.
            void loadStaticGrid(GameObject::Map& _outObjects,
                int _gridX,
                int _gridZ,
                float _spacing,
                float _uniformScale,
                float _y,
                bool _lights = true
            );

            void loadMovingScene(GameObject::Map& _outObjects,
                int _gridX, int _gridZ,
                float _spacing,
                float _uniformScale,
                float _y,
                bool _lights = true
            );

            void updateMovingScene(float _dt, GameObject::Map& _outObjects);

            void loadTransparencyTest(GameObject::Map& _outObjects,
                int _quads, float _y, float _s
            );

            SceneType m_sceneType;

        private:
            struct Mover 
            {
                glm::vec3 base;      // base position
                float ax, az, ay;    // amplitudes
                float fx, fz, fy;    // angular frequencies
                float phx, phz, phy; // phases
                float rotSpeed;      // radians/sec around Y
            };

            std::unordered_map<GameObject::id_t, Mover> m_movers;
            float m_time = 0.0f;

            EngineDevice& m_device;

            void addDefaultLights(GameObject::Map& _outObjects,
                int _count,
                float _intensity,
                float _radius
            );
        };

        struct CameraPanController
        {
            float m_radius = 8.5f;  // orbit radius
            float m_height = -2.5f;  // eye height
            float m_angularSpeed = 0.7f;
            glm::vec3 m_target = { 0.f, -0.0f, 0.f };
            float m_time = 0.f;

            void update(float _dt, GameObject& _viewer)
            {
                m_time += _dt * m_angularSpeed;
                const float x = m_radius * std::sin(m_time);
                const float z = -m_radius * std::cos(m_time);
                _viewer.m_transform.m_translation = { x, m_height, z };

                const glm::vec3 toTarget = glm::normalize(m_target - _viewer.m_transform.m_translation);
                const float yaw = std::atan2(toTarget.x, toTarget.z);
                const float pitch = std::asin(-toTarget.y);
                _viewer.m_transform.m_rotation = { pitch, yaw, 0.0f };
            }
        };
    };
}