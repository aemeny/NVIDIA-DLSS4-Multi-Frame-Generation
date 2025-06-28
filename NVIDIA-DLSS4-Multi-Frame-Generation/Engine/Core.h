#pragma once
#include "Pipeline.h"
#include "SwapChain.h"
#include "GameObject.h"

#include <memory>

namespace Engine
{
    struct Core 
    {
        // Window dimensions
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        Core(std::shared_ptr<EngineWindow> _window);
        ~Core();
        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        void run();
        void stop();

    private:
        bool m_terminateApplication;
        void drawFrame();
        void renderGameObjects(VkCommandBuffer _commandBuffer);

        // Member objs
        std::shared_ptr<EngineWindow> m_window;
        EngineDevice m_device{ m_window };

        void loadGameObjects();
        std::vector<GameObject> m_gameObjects;

        //Swap Chain
        void recreateSwapChain();
        void recordCommandBuffers(int _imageIndex);
        std::unique_ptr<SwapChain> m_swapChain;

        // Pipeline
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void freeCommandBuffers();
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        std::vector<VkCommandBuffer> m_commandBuffers;
    };
}