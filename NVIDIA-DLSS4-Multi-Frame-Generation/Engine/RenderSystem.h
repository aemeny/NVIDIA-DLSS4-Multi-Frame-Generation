#pragma once
#include "Pipeline.h"
#include "GameObject.h"
#include "FrameInfo.h"

namespace Engine
{
    struct RenderSystem
    {
        RenderSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout);
        ~RenderSystem();
        RenderSystem(const RenderSystem&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;

        void renderGameObjects(FrameInfo& _frameInfo);

    private:
        void createPipeline(VkRenderPass _renderPass);
        void createPipelineLayout(VkDescriptorSetLayout _globalSetLayout);
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        EngineDevice& m_device;
    };
}