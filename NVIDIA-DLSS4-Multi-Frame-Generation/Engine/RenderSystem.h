#pragma once
#include "Pipeline.h"
#include "GameObject.h"
#include "FrameInfo.h"

namespace Engine
{
    struct RenderSystem
    {
        RenderSystem(EngineDevice& _device, VkRenderPass _renderPass);
        ~RenderSystem();
        RenderSystem(const RenderSystem&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;

        void renderGameObjects(FrameInfo& _frameInfo, std::vector<GameObject>& _gameObjects);

    private:
        void createPipeline(VkRenderPass _renderPass);
        void createPipelineLayout();
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        EngineDevice& m_device;
    };
}