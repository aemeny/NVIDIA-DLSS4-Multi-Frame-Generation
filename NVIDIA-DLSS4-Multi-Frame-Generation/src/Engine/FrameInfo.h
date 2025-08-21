#pragma once
#include "Camera.h"
#include "GameObject.h"
#include "descriptors.h"

#include <vulkan/vulkan.h>

namespace Engine
{
    #define MAX_LIGHTS 10

    struct PointLight 
    {
        // Both vec4 for simple memory alignment
        glm::vec4 m_position{}; // Ignore W
        glm::vec4 m_color{}; // W is intensity
    };

    struct GlobalUbo
    {
        glm::mat4 m_projection{ 1.0f };
        glm::mat4 m_view{ 1.0f };
        glm::mat4 m_prevView;
        glm::mat4 m_prevProjection;
        glm::mat4 m_inverseView{1.0f}; // Inverse for lighting calculations
        glm::vec4 m_ambientLight{ 1.0f, 1.0f, 1.0f, 0.02f }; // W is intensity
        PointLight m_pointLights[MAX_LIGHTS]; // Array of point lights
        glm::vec2 m_renderSize;
        int m_numLights = 0; // Number of active lights
    };

    struct FrameInfo 
    {
        int m_frameIndex;
        float m_frameTime;
        VkCommandBuffer m_commandBuffer;
        Camera& m_camera;
        VkDescriptorSet m_globalDescriptorSet;
        DescriptorPool& m_frameDescriptorPool;
        GameObject::Map& m_gameObjects;
    };
}