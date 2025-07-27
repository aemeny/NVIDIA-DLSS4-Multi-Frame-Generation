#include "RenderSystem.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include "TextureRenderSystem.h"

namespace Engine
{
    struct SimplePushConstantData
    {
        glm::mat4 m_modelMatrix{ 1.0f };
        glm::mat4 m_normalMatrix{ 1.0f };
    };


    RenderSystem::RenderSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout)
        : m_device(_device)
    {
        createPipelineLayout(_globalSetLayout);
        createPipeline(_renderPass);
    }

    RenderSystem::~RenderSystem()
    {
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout(VkDescriptorSetLayout _globalSetLayout)
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {_globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
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
        m_pipeline = std::make_unique<Pipeline>(m_device, "Shaders/Basic/Vertex.vert.spv", "Shaders/Basic/Fragment.frag.spv", pipelineConfig);
    }

    void RenderSystem::renderGameObjects(FrameInfo& _frameInfo)
    {
        m_pipeline->bind(_frameInfo.m_commandBuffer);

        vkCmdBindDescriptorSets(
            _frameInfo.m_commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0, 1,
            &_frameInfo.m_globalDescriptorSet,
            0, nullptr
        );

        for (auto& keyValue : _frameInfo.m_gameObjects)
        {
            GameObject& obj = keyValue.second;
            if (obj.m_model == nullptr || obj.m_diffuseMap != nullptr) continue;

            SimplePushConstantData push = {};
            push.m_modelMatrix = obj.m_transform.mat4();
            push.m_normalMatrix = obj.m_transform.normalMatrix();

            vkCmdPushConstants(_frameInfo.m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);

            obj.m_model->bind(_frameInfo.m_commandBuffer);
            obj.m_model->draw(_frameInfo.m_commandBuffer);
        }
    }
}