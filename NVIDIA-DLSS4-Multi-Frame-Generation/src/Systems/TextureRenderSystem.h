#pragma once

#include "../Engine/FrameInfo.h"
#include "../Engine/GameObject.h"
#include "../Engine/Pipeline.h"

#include <memory>
#include <vector>

namespace Engine
{
    struct TextureRenderSystem
    {
        TextureRenderSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout);
        ~TextureRenderSystem();

        TextureRenderSystem(const TextureRenderSystem&) = delete;
        TextureRenderSystem& operator=(const TextureRenderSystem&) = delete;

        void renderGameObjects(FrameInfo& _frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout _globalSetLayout);
        void createPipeline(VkRenderPass _renderPass);

        EngineDevice& m_device;

        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        std::unique_ptr<DescriptorSetLayout> m_renderSystemLayout;
    };
}