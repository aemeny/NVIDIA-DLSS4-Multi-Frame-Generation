#include "RenderSystem.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>

namespace Engine
{
    struct SimplePushConstantData
    {
        glm::mat4 m_transform{ 1.0f };
        glm::mat4 m_normalMatrix{ 1.0f };
    };


    RenderSystem::RenderSystem(EngineDevice& _device, VkRenderPass _renderPass)
        : m_device(_device)
    {
        createPipelineLayout();
        createPipeline(_renderPass);
    }

    RenderSystem::~RenderSystem()
    {
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout()
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
    }

    void RenderSystem::createPipeline(VkRenderPass _renderPass)
    {
        assert(m_pipelineLayout != nullptr && "Cannot create graphics pipeline: No pipelineLayout provided");

        PipelineConfigInfo pipelineConfig = {};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.m_renderPass = _renderPass;
        pipelineConfig.m_pipelineLayout = m_pipelineLayout;
        m_pipeline = std::make_unique<Pipeline>(m_device, "Shaders/Unlit/Vertex.vert.spv", "Shaders/Unlit/Fragment.frag.spv", pipelineConfig);
    }

    void RenderSystem::renderGameObjects(FrameInfo& _frameInfo, std::vector<GameObject>& _gameObjects)
    {
        m_pipeline->bind(_frameInfo.m_commandBuffer);

        glm::mat4 projectView = _frameInfo.m_camera.getProjectionMatrix() * _frameInfo.m_camera.getViewMatrix();

        for (GameObject& obj : _gameObjects)
        {
            SimplePushConstantData push = {};
            glm::mat4 modelMatrix = obj.m_transform.mat4();
            push.m_normalMatrix = obj.m_transform.normalMatrix();
            push.m_transform = projectView * modelMatrix;

            vkCmdPushConstants(_frameInfo.m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);

            obj.m_model->bind(_frameInfo.m_commandBuffer);
            obj.m_model->draw(_frameInfo.m_commandBuffer);
        }
    }
}