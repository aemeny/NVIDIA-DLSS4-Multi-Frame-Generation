#pragma once
#include "Camera.h"
#include "GameObject.h"

#include <vulkan/vulkan.h>

namespace Engine
{
    struct FrameInfo 
    {
        int m_frameIndex;
        float m_frameTime;
        VkCommandBuffer m_commandBuffer;
        Camera& m_camera;
        VkDescriptorSet m_globalDescriptorSet;
        GameObject::Map& m_gameObjects;
    };
}