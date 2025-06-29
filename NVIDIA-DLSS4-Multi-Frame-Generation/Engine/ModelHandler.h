#pragma once
#include "EngineDevice.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Engine
{
    struct Model 
    {
        struct Vertex
        {
            glm::vec3 m_position;
            glm::vec3 m_colour;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        Model(EngineDevice& _device, const std::vector<Vertex>& _vertices);
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        void bind(VkCommandBuffer _commandBuffer);
        void draw(VkCommandBuffer _commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex>& _vertices);

        EngineDevice& m_device;

        VkBuffer m_vertexBuffer;
        VkDeviceMemory m_vertexBufferMemory;
        uint32_t m_vertexCount;
    };
}