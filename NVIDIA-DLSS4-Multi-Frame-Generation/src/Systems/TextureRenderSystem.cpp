#include "TextureRenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cassert>
#include <stdexcept>

namespace Engine
{
    struct TexturePushConstantData 
    {
        glm::mat4 m_modelMatrix{ 1.0f };
        glm::mat4 m_normalMatrix{ 1.0f };
        glm::mat4 m_prevModelMatrix{ 1.0f }; // Previous frame model matrix for motion vectors
    };


    TextureRenderSystem::TextureRenderSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout) :
        m_device(_device)
    {
        createPipelineLayout(_globalSetLayout);
        createPipeline(_renderPass);
    }

    TextureRenderSystem::~TextureRenderSystem()
    {
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void TextureRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) 
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(TexturePushConstantData);

        m_renderSystemLayout =
            DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            globalSetLayout,
            m_renderSystemLayout->getDescriptorSetLayout() 
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void TextureRenderSystem::createPipeline(VkRenderPass renderPass) 
    {
        assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.m_renderPass = renderPass;
        pipelineConfig.m_pipelineLayout = m_pipelineLayout;

        Pipeline::enableAlphaBlending(pipelineConfig);

        //pipelineConfig.m_depthStencilInfo.depthTestEnable = VK_FALSE;
        //pipelineConfig.m_depthStencilInfo.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colourBlendAttachments[2] = {
            pipelineConfig.m_colorBlendAttachment,
            pipelineConfig.m_colorBlendAttachment
        };
        colourBlendAttachments[1].blendEnable = VK_FALSE;
        pipelineConfig.m_colorBlendInfo.attachmentCount = 2;
        pipelineConfig.m_colorBlendInfo.pAttachments = colourBlendAttachments;

        m_pipeline = std::make_unique<Pipeline>(m_device, "Shaders/TextureShader.vert.spv", "Shaders/TextureShader.frag.spv", pipelineConfig);
    }

    void TextureRenderSystem::renderGameObjects(FrameInfo& _frameInfo) 
    {
        m_pipeline->bind(_frameInfo.m_commandBuffer);

        vkCmdBindDescriptorSets(
            _frameInfo.m_commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &_frameInfo.m_globalDescriptorSet,
            0,
            nullptr);

        for (auto& kv : _frameInfo.m_gameObjects) 
        {
            auto& obj = kv.second;

            if (obj.m_model == nullptr || obj.m_diffuseMap == nullptr) continue;

            auto imageInfo = obj.m_diffuseMap->getImageInfo();
            VkDescriptorSet descriptorSet1;
            DescriptorWriter(*m_renderSystemLayout, _frameInfo.m_frameDescriptorPool)
                .writeImage(0, &imageInfo)
                .build(descriptorSet1);

            vkCmdBindDescriptorSets(
                _frameInfo.m_commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                1,  // first set
                1,  // set count
                &descriptorSet1,
                0,
                nullptr);

            TexturePushConstantData push{};
            push.m_modelMatrix = obj.m_transform.mat4();
            push.m_normalMatrix = obj.m_transform.normalMatrix();
            push.m_prevModelMatrix = obj.m_transform.m_prevModelMatrix;

            vkCmdPushConstants(
                _frameInfo.m_commandBuffer,
                m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(TexturePushConstantData),
                &push);

            obj.m_model->bind(_frameInfo.m_commandBuffer);
            obj.m_model->draw(_frameInfo.m_commandBuffer);
        }
    }
}