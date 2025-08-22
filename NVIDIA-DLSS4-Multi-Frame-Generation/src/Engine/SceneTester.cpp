#include "SceneTester.h"
#include <algorithm>

namespace Engine
{
    void SceneTester::SceneLoader::addDefaultLights(GameObject::Map& _outObjects,int _count, float _intensity, float _radius)
    {
        // Cap to MAX_LIGHTS = 10
        _count = std::max(0, std::min(_count, 10));

        std::vector<glm::vec3> lightColors{
            {1.0f, 0.0f, 0.0f},   // Red
            {1.0f, 0.35f, 0.0f},  // Orange
            {1.0f, 0.8f, 0.0f},   // Yellow
            {0.0f, 1.0f, 0.0f},   // Green
            {0.0f, 0.9f, 0.6f},   // Teal
            {0.0f, 0.0f, 1.0f},   // Blue
            {0.4f, 0.0f, 1.0f},   // Indigo
            {0.75f, 0.0f, 1.0f},  // Violet
            {1.0f, 0.0f, 0.6f},   // Magenta
            {1.0f, 0.4f, 0.7f}    // Pink
        };

        const float orbitR = 6.0f;
        for (int i = 0; i < _count; i++) 
        {
            GameObject light = GameObject::makePointLight(_intensity, _radius); // radius -> transform.scale.x
            light.m_colour = lightColors[i % lightColors.size()];
            float t = (i / float(_count)) * glm::two_pi<float>();
            light.m_transform.m_translation = { orbitR * std::cos(t), -1.2f, orbitR * std::sin(t) };
            _outObjects.emplace(light.getId(), std::move(light));
        }
    }

    void SceneTester::SceneLoader::loadStaticGrid(GameObject::Map& _outObjects, int _gridX, int _gridZ, float _spacing, float _uniformScale, float _y, bool _lights)
    {
        _outObjects.clear();

        // One shared model for all instances (VRAM/CPU friendly)
        const std::string modelPath = "Samples/Models/flat_vase.obj";
        std::shared_ptr<Model> model = Model::createModelFromFile(m_device, modelPath);
        std::shared_ptr<Texture> texture = Texture::createTextureFromFile(m_device, "Samples/Textures/Curuthers.png");

        const float halfX = (_gridX - 1) * 0.5f;
        const float halfZ = (_gridZ - 1) * 0.5f;

        for (int z = 0; z < _gridZ; z++) 
        {
            for (int x = 0; x < _gridX; x++)
            {
                GameObject obj = GameObject::createGameObject();
                obj.m_model = model;
                obj.m_diffuseMap = texture;

                const float px = (x - halfX) * _spacing;
                const float pz = (z - halfZ) * _spacing;

                obj.m_transform.m_translation = { px, _y, pz };
                obj.m_transform.m_scale = { _uniformScale, _uniformScale, _uniformScale };
                obj.m_transform.m_rotation = { 0.0f, 0.0f, 0.0f };
                obj.m_transform.m_prevModelMatrix = obj.m_transform.mat4();

                _outObjects.emplace(obj.getId(), std::move(obj));
            }
        }

        if (_lights)
        {
            addDefaultLights(_outObjects, 
                10, // Count
                3.0f, // Intensity
                10.0f // Radius
            );
        }
    }

    static inline float hash01(uint32_t v) {
        // simple xorshift -> [0,1)
        v ^= v << 13; v ^= v >> 17; v ^= v << 5;
        return (v * 0x9E3779B9u) / float(0xFFFFFFFFu);
    }

    void SceneTester::SceneLoader::loadMovingScene(GameObject::Map& _outObjects, int _gridX, int _gridZ, float _spacing, float _uniformScale, float _y, bool _lights)
    {
        _outObjects.clear();
        m_movers.clear();
        m_time = 0.0f;

        // Shared model + (optional) texture
        //const std::string modelPath = "Samples/Models/coloured_cube.obj";
        const std::string modelPath = "Samples/Models/flat_vase.obj";
        std::shared_ptr<Model> model = Model::createModelFromFile(m_device, modelPath);
        std::shared_ptr<Texture> texture = Texture::createTextureFromFile(m_device, "Samples/Textures/Curuthers.png");

        const float halfX = (_gridX - 1) * 0.5f;
        const float halfZ = (_gridZ - 1) * 0.5f;

        // Spawn grid of movers
        for (int z = 0; z < _gridZ; z++) 
        {
            for (int x = 0; x < _gridX; x++) 
            {
                GameObject obj = GameObject::createGameObject();
                obj.m_model = model;
                obj.m_diffuseMap = texture;

                const float px = (x - halfX) * _spacing;
                const float pz = (z - halfZ) * _spacing;

                obj.m_transform.m_translation = { px, _y, pz };
                obj.m_transform.m_scale = { _uniformScale, _uniformScale, _uniformScale };
                obj.m_transform.m_rotation = { 0.0f, 0.0f, 0.0f };
                obj.m_transform.m_prevModelMatrix = obj.m_transform.mat4();

                _outObjects.emplace(obj.getId(), std::move(obj));

                // Per-object motion params (deterministic)
                uint32_t seed = (uint32_t)(x * 73856093) ^ (uint32_t)(z * 19349663);
                float hx = hash01(seed ^ 0x00000001);
                float hz = hash01(seed ^ 0x00000002);
                float hy = hash01(seed ^ 0x00000003);
                float fx = hash01(seed ^ 0x00000004);
                float fz = hash01(seed ^ 0x00000005);
                float fy = hash01(seed ^ 0x00000006);
                float phx = hash01(seed ^ 0x00000007) * glm::two_pi<float>();
                float phz = hash01(seed ^ 0x00000008) * glm::two_pi<float>();
                float phy = hash01(seed ^ 0x00000009) * glm::two_pi<float>();
                float rs = 0.4f + hash01(seed ^ 0x00000010) * 1.2f;

                Mover m;
                m.base = { px, _y, pz };
                // amplitudes
                m.ax = 0.35f + 0.35f * hx;
                m.az = 0.35f + 0.35f * hz;
                m.ay = 0.06f + 0.10f * hy;
                // angular frequencies
                m.fx = 0.6f + 1.4f * fx;
                m.fz = 0.6f + 1.4f * fz;
                m.fy = 0.4f + 0.8f * fy;
                m.phx = phx; m.phz = phz; m.phy = phy;
                m.rotSpeed = rs;

                m_movers.emplace(obj.getId(), m);
            }
        }

        if (_lights)
        {
            addDefaultLights(_outObjects,
                10, // Count
                3.0f, // Intensity
                10.0f // Radius
            );
        }
    }

    void SceneTester::SceneLoader::updateMovingScene(float _dt, GameObject::Map& _outObjects)
    {
        if (m_movers.empty()) return;
        m_time += _dt;

        for (auto& [id, obj] : _outObjects) 
        {
            if (!obj.m_model) continue;

            auto it = m_movers.find(id);
            if (it == m_movers.end()) continue;
            const Mover& m = it->second;

            glm::vec3 p = m.base;
            p.x += m.ax * std::sin(m.fx * m_time + m.phx);
            p.z += m.az * std::cos(m.fz * m_time + m.phz);
            p.y += m.ay * std::sin(m.fy * m_time + m.phy);
            obj.m_transform.m_translation = p;

            obj.m_transform.m_rotation.y += m.rotSpeed * _dt;
        }
    }

    void SceneTester::SceneLoader::loadTransparencyTest(GameObject::Map& _outObjects,
        int _quads, float _y, float _s)
    {
        _outObjects.clear();

        // One shared quad + alpha-capable texture (use a sprite with real alpha if possible)
        std::shared_ptr<Model> quad = Model::createModelFromFile(m_device, "Samples/Models/quad.obj");
        std::shared_ptr<Texture> texture = Texture::createTextureFromFile(m_device, "Samples/Textures/Curuthers.png");

        const float turns = 4.0f; // Total rotations across all quads
        const float rStep = 0.005f; // Growth per quad
        const float pitchPerQuad = 0.015f;  // Rise per quad

        for (int i = 0; i < _quads; ++i)
        {
            GameObject q = GameObject::createGameObject();
            q.m_model = quad;
            q.m_diffuseMap = texture;

            const float t = (_quads > 1) ? (float)i / (float)(_quads - 1) : 0.0f;
            const float a = t * turns * glm::two_pi<float>();
            const float r = rStep * i; 
            const float y = _y + pitchPerQuad * i;

            q.m_transform.m_translation = { r * std::cos(a), y, r * std::sin(a) };
            q.m_transform.m_rotation = { a, glm::radians(90.f), 0.0f };
            q.m_transform.m_scale = { _s, 1.f, _s };

            q.m_transform.m_prevModelMatrix = q.m_transform.mat4();

            _outObjects.emplace(q.getId(), std::move(q));
        }
    }
}