#include "RenderSystem.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>

namespace Engine
{
    struct SimplePushConstantData
    {
        glm::mat4 m_transform{ 1.0f };
        alignas(16) glm::vec3 m_colour;
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

    void RenderSystem::renderGameObjects(VkCommandBuffer _commandBuffer, std::vector<GameObject>& _gameObjects, const Camera& _camera)
    {
        m_pipeline->bind(_commandBuffer);

        for (GameObject& obj : _gameObjects)
        {
            obj.m_transform.m_rotation.x = glm::mod(obj.m_transform.m_rotation.x + 0.0001f, glm::two_pi<float>());
            obj.m_transform.m_rotation.y = glm::mod(obj.m_transform.m_rotation.y + 0.0002f, glm::two_pi<float>());

            SimplePushConstantData push = {};
            push.m_colour = obj.m_colour;
            push.m_transform = _camera.getProjectionMatrix() * obj.m_transform.mat4();

            vkCmdPushConstants(_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);

            obj.m_model->bind(_commandBuffer);
            obj.m_model->draw(_commandBuffer);
        }
    }
}