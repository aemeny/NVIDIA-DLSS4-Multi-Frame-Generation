#include "ModelHandler.h"

#include <cassert>
#include <cstring>

namespace Engine
{
    Model::Model(EngineDevice& _device, const std::vector<Vertex>& _vertices)
        : m_device(_device)
    {
        createVertexBuffers(_vertices);
    }

    Model::~Model()
    {
        vkDestroyBuffer(m_device.device(), m_vertexBuffer, nullptr);
        vkFreeMemory(m_device.device(), m_vertexBufferMemory, nullptr);
    }

    void Model::createVertexBuffers(const std::vector<Vertex>& _vertices)
    {
        m_vertexCount = static_cast<uint32_t>(_vertices.size());
        assert(m_vertexCount >= 3 && "Vertex count must be at least 3");

        VkDeviceSize bufferSize = sizeof(_vertices[0]) * m_vertexCount;

        m_device.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_vertexBuffer,
            m_vertexBufferMemory
        );

        void* data;
        vkMapMemory(m_device.device(), m_vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, _vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(m_device.device(), m_vertexBufferMemory);
    }

    void Model::draw(VkCommandBuffer _commandBuffer)
    {
        vkCmdDraw(_commandBuffer, m_vertexCount, 1, 0, 0);
    }

    void Model::bind(VkCommandBuffer _commandBuffer)
    {
        VkBuffer buffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, buffers, offsets);
    }

    std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions()
    {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0; // Binding index
        bindingDescriptions[0].stride = sizeof(Vertex); // Size of each vertex
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Each vertex has its own data

        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

        // Position attribute
        attributeDescriptions[0].binding = 0; // Binding index
        attributeDescriptions[0].location = 0; // Location in the shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // Format of the vertex data
        attributeDescriptions[0].offset = offsetof(Vertex, m_position);

        // Colour attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, m_colour);

        return attributeDescriptions;
    }
}