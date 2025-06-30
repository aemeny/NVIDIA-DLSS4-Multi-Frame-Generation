#pragma once
#include "SwapChain.h"

#include <memory>
#include <cassert>

namespace Engine
{
    struct Renderer
    {
        Renderer(std::weak_ptr<EngineWindow> _window, EngineDevice& _device);
        ~Renderer();
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        VkRenderPass getSwapChainRenderPass() const { return m_swapChain->getRenderPass(); }
        float getAspectRatio() const { return m_swapChain->extentAspectRatio(); }
        bool isFrameInProgress() const { return m_isFrameStarted; }
        VkCommandBuffer getCurrentCommandBuffer() const { 
            assert(m_isFrameStarted && "Cannot get command buffer when frame is not in progress!");
            return m_commandBuffers[m_currentFrameIndex];
        };
        int getCurrentFrameIndex() const { 
            assert(m_isFrameStarted && "Cannot get current frame index when frame is not in progress!");
            return m_currentFrameIndex; 
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer _commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer _commandBuffer);

    private:
        // Member objs
        std::weak_ptr<EngineWindow> m_window;
        EngineDevice& m_device;

        //Swap Chain
        void recreateSwapChain();
        std::unique_ptr<SwapChain> m_swapChain;
        uint32_t m_currentImageIndex;
        int m_currentFrameIndex = 0;
        bool m_isFrameStarted = false;

        // Pipeline
        void createCommandBuffers();
        void freeCommandBuffers();
        std::vector<VkCommandBuffer> m_commandBuffers;
    };
}