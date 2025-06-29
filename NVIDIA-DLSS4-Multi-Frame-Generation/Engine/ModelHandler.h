#pragma once
#include "EngineDevice.h"
#include "Buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Engine
{
    struct Model 
    {
        struct Vertex
        {
            glm::vec3 m_position{};
            glm::vec3 m_colour{};
            glm::vec3 m_normal{};
            glm::vec2 m_uv{};

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const
            {
                return m_position == other.m_position &&
                       m_colour == other.m_colour &&
                       m_normal == other.m_normal &&
                       m_uv == other.m_uv;
            }
        };
        struct Data
        {
            std::vector<Vertex> m_vertices{};
            std::vector<uint32_t> m_indices{};

            void loadModel(const std::string& _filePath);
        };

        Model(EngineDevice& _device, const Model::Data& _data);
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        static std::unique_ptr<Model> createModelFromFile(EngineDevice& _device, const std::string& _filePath);

        void bind(VkCommandBuffer _commandBuffer);
        void draw(VkCommandBuffer _commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex>& _vertices);
        void createIndexBuffer(const std::vector<uint32_t>& _indices);

        EngineDevice& m_device;

        std::unique_ptr<Buffer> m_vertexBuffer;
        uint32_t m_vertexCount;

        bool hasIndexBuffer = false;
        std::unique_ptr<Buffer> m_indexBuffer;
        uint32_t m_indexCount;
    };
}