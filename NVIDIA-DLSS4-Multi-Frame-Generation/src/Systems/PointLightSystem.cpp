#include "PointLightSystem.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <map>

namespace Engine
{
    struct PointLightPushConstants
    {
        glm::vec4 m_position{};
        glm::vec4 m_color{};
        float radius;
    };

    PointLightSystem::PointLightSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout)
        : m_device(_device)
    {
        createPipelineLayout(_globalSetLayout);
        createPipeline(_renderPass);
    }

    PointLightSystem::~PointLightSystem()
    {
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout _globalSetLayout)
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstants);

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

    void PointLightSystem::createPipeline(VkRenderPass _renderPass)
    {
        assert(m_pipelineLayout != nullptr && "Cannot create graphics pipeline: No pipelineLayout provided");

        PipelineConfigInfo pipelineConfig = {};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        Pipeline::enableAlphaBlending(pipelineConfig);
        pipelineConfig.m_attributeDescriptions.clear();
        pipelineConfig.m_bindingDescriptions.clear();
        pipelineConfig.m_renderPass = _renderPass;
        pipelineConfig.m_pipelineLayout = m_pipelineLayout;

        VkPipelineColorBlendAttachmentState colourBlendAttachments[2] = {
           pipelineConfig.m_colorBlendAttachment,
           pipelineConfig.m_colorBlendAttachment
        };
        colourBlendAttachments[1].blendEnable = VK_FALSE;
        pipelineConfig.m_colorBlendInfo.attachmentCount = 2;
        pipelineConfig.m_colorBlendInfo.pAttachments = colourBlendAttachments;

        m_pipeline = std::make_unique<Pipeline>(m_device, "Shaders/PointLight.vert.spv", "Shaders/PointLight.frag.spv", pipelineConfig);
    }

    void PointLightSystem::update(FrameInfo& _frameInfo, GlobalUbo& _globalUbo, bool _rotate)
    {
        float rotateAngle = _rotate ? _frameInfo.m_frameTime : 0;
        auto rotateLight = glm::rotate(
            glm::mat4(1.0f),
            rotateAngle,
            glm::vec3(0.0f, -1.0f, 0.0f));

        int lightIndex = 0;
        for (auto& keyValue : _frameInfo.m_gameObjects)
        {
            GameObject& gameObject = keyValue.second;
            if (gameObject.m_pointLight == nullptr) continue;

            assert(lightIndex < MAX_LIGHTS && "Exceeded maximum number of point lights!");

            // Update light pos
            gameObject.m_transform.m_translation = glm::vec3(rotateLight * glm::vec4(gameObject.m_transform.m_translation, 1.0f));

            // Copt light to ubo
            _globalUbo.m_pointLights[lightIndex].m_position = glm::vec4(gameObject.m_transform.m_translation, 1.0f);
            _globalUbo.m_pointLights[lightIndex].m_color = glm::vec4(gameObject.m_colour, gameObject.m_pointLight->m_intensity);

            lightIndex++;
        }
        _globalUbo.m_numLights = lightIndex;
    }

    void PointLightSystem::render(FrameInfo& _frameInfo)
    {
        // Sort lights
        std::map<float, GameObject::id_t> sortedLights;
        for (auto& keyValue : _frameInfo.m_gameObjects)
        {
            GameObject& gameObject = keyValue.second;
            if (gameObject.m_pointLight == nullptr) continue;

            auto offset = _frameInfo.m_camera.getPosition() - gameObject.m_transform.m_translation;
            float distanceSquared = glm::dot(offset, offset);
            sortedLights[distanceSquared] = gameObject.getId();
        }

        m_pipeline->bind(_frameInfo.m_commandBuffer);

        vkCmdBindDescriptorSets(
            _frameInfo.m_commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0, 1,
            &_frameInfo.m_globalDescriptorSet,
            0, nullptr
        );

        // Iterate through sorted lights in reverse order (furthest to closest)
        for (auto it = sortedLights.rbegin(); it != sortedLights.rend(); ++it)
        {
            GameObject& gameObject = _frameInfo.m_gameObjects.at(it->second);

            PointLightPushConstants pushConstants = {};
            pushConstants.m_position = glm::vec4(gameObject.m_transform.m_translation, 1.0f);
            pushConstants.m_color = glm::vec4(gameObject.m_colour, gameObject.m_pointLight->m_intensity);
            pushConstants.radius = gameObject.m_transform.m_scale.x;

            vkCmdPushConstants(
                _frameInfo.m_commandBuffer,
                m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PointLightPushConstants),
                &pushConstants
            );

            vkCmdDraw(_frameInfo.m_commandBuffer, 6, 1, 0, 0);
        }
    }
}