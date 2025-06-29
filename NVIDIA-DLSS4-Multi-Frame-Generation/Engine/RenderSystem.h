#pragma once
#include "Pipeline.h"
#include "GameObject.h"
#include "Camera.h"

namespace Engine
{
    struct RenderSystem
    {
        RenderSystem(EngineDevice& _device, VkRenderPass _renderPass);
        ~RenderSystem();
        RenderSystem(const RenderSystem&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;

        void renderGameObjects(VkCommandBuffer _commandBuffer, std::vector<GameObject>& _gameObjects, const Camera& _camera);

    private:
        void createPipeline(VkRenderPass _renderPass);
        void createPipelineLayout();
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        EngineDevice& m_device;
    };
}