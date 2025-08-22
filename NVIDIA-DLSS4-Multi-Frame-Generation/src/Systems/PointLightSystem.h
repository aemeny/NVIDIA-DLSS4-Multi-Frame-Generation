#pragma once
#include "..\Engine\Pipeline.h"
#include "..\Engine\GameObject.h"
#include "..\Engine\FrameInfo.h"

namespace Engine
{
    struct PointLightSystem
    {
        PointLightSystem(EngineDevice& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout);
        ~PointLightSystem();
        PointLightSystem(const PointLightSystem&) = delete;
        PointLightSystem& operator=(const PointLightSystem&) = delete;

        void update(FrameInfo& _frameInfo, GlobalUbo& _globalUbo, bool _rotate);
        void render(FrameInfo& _frameInfo);

    private:
        void createPipeline(VkRenderPass _renderPass);
        void createPipelineLayout(VkDescriptorSetLayout _globalSetLayout);
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        EngineDevice& m_device;
    };
}