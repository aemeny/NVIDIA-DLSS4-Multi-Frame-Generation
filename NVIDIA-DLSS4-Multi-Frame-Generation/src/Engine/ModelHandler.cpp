#include "ModelHandler.h"
#include "Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjectloader/tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <unordered_map>

// Turns single Vertex into a hash at size_t in order to use it in unordered_map
namespace std
{
    template<>struct 
    hash<Engine::Model::Vertex>
    {
        size_t operator()(Engine::Model::Vertex const& _vertex) const
        {
            size_t seed = 0;
            Engine::hashCombine(seed, _vertex.m_position, _vertex.m_colour, _vertex.m_normal, _vertex.m_uv);
            return seed;
        }
    };
}

namespace Engine
{
    Model::Model(EngineDevice& _device, const Model::Data& _data)
        : m_device(_device)
    {
        createVertexBuffers(_data.m_vertices);
        createIndexBuffer(_data.m_indices);
    }

    Model::~Model(){}

    void Model::createVertexBuffers(const std::vector<Vertex>& _vertices)
    {
        m_vertexCount = static_cast<uint32_t>(_vertices.size());
        assert(m_vertexCount >= 3 && "Vertex count must be at least 3");

        VkDeviceSize bufferSize = sizeof(_vertices[0]) * m_vertexCount;
        uint32_t vertexSize = sizeof(_vertices[0]);

        Buffer stagingBuffer{
            m_device,
            vertexSize,
            m_vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)_vertices.data());

        m_vertexBuffer = std::make_unique<Buffer>(
            m_device,
            vertexSize,
            m_vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
    }

    void Model::createIndexBuffer(const std::vector<uint32_t>& _indices)
    {
        m_indexCount = static_cast<uint32_t>(_indices.size());
        hasIndexBuffer = m_indexCount > 0;
        if (!hasIndexBuffer) return; // No index buffer to create

        VkDeviceSize bufferSize = sizeof(_indices[0]) * m_indexCount;
        uint32_t indexSize = sizeof(_indices[0]);

        Buffer stagingBuffer{
            m_device,
            indexSize,
            m_indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)_indices.data());

        m_indexBuffer = std::make_unique<Buffer>(
            m_device,
            indexSize,
            m_indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
    }

    void Model::draw(VkCommandBuffer _commandBuffer)
    {
        if (hasIndexBuffer)
            vkCmdDrawIndexed(_commandBuffer, m_indexCount, 1, 0, 0, 0);
        else
            vkCmdDraw(_commandBuffer, m_vertexCount, 1, 0, 0);
    }

    std::unique_ptr<Model> Model::createModelFromFile(EngineDevice& _device, const std::string& _filePath)
    {
        Data data;
        data.loadModel(_filePath);
        return std::make_unique<Model>(_device, data);
    }

    void Model::bind(VkCommandBuffer _commandBuffer)
    {
        VkBuffer buffers[] = { m_vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer)
            vkCmdBindIndexBuffer(_commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        // Position attribute
        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_position)});
        // Colour attribute
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_colour)});
        // Normal attribute
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_normal)});
        // UV attribute
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_uv)});

        return attributeDescriptions;
    }

    void Model::Data::loadModel(const std::string& _filePath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _filePath.c_str()))
            throw std::runtime_error("Failed to load model: " + warn + err);

        m_vertices.clear();
        m_indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                if (index.vertex_index >= 0)
                {
                    vertex.m_position = { 
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.m_colour = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2]
                    };
                }

                if (index.normal_index >= 0)
                {
                    vertex.m_normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (index.texcoord_index >= 0)
                {
                    vertex.m_uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                // If new Vertex add it to the uniqueVertices map
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
}