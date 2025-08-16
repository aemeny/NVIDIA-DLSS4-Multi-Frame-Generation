#pragma once
#include "EngineDevice.h"

#include <vulkan/vulkan.h>
#include <memory>

namespace Engine
{
    struct SwapChain 
    {
        static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

        SwapChain(EngineDevice& _deviceRef, VkExtent2D _windowExtent);
        SwapChain(EngineDevice& _deviceRef, VkExtent2D _windowExtent, std::shared_ptr<SwapChain> _previous);
        ~SwapChain();

        SwapChain(const SwapChain&) = delete;
        void operator=(const SwapChain&) = delete;

        VkFramebuffer getFrameBuffer(int _index) { return m_swapChainFramebuffers[_index]; }
        VkRenderPass getRenderPass() { return m_renderPass; }
        VkImageView getImageView(int index) { return m_swapChainImageViews[index]; }
        size_t imageCount() { return m_swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() { return m_swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() { return m_swapChainExtent; }
        uint32_t width() { return m_swapChainExtent.width; }
        uint32_t height() { return m_swapChainExtent.height; }

        float extentAspectRatio() { return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height); }
        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t* _imageIndex);
        VkResult submitCommandBuffers(const VkCommandBuffer* _buffers, uint32_t* _imageIndex);

        bool compareSwapFormats(const SwapChain& _swapChain) const {
            return _swapChain.m_swapChainDepthFormat == m_swapChainDepthFormat && 
                   _swapChain.m_swapChainImageFormat == m_swapChainImageFormat;
        }

    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createDepthResources();
        void createRenderPass();
        void createFramebuffers();
        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);

        VkFormat m_swapChainImageFormat;
        VkFormat m_swapChainDepthFormat;
        VkExtent2D m_swapChainExtent;

        std::vector<VkFramebuffer> m_swapChainFramebuffers;
        VkRenderPass m_renderPass;

        std::vector<VkImage> m_depthImages;
        std::vector<VkDeviceMemory> m_depthImageMemories;
        std::vector<VkImageView> m_depthImageViews;
        std::vector<VkImage> m_swapChainImages;
        std::vector<VkImageView> m_swapChainImageViews;

        EngineDevice& m_device;
        VkExtent2D m_windowExtent;

        VkSwapchainKHR m_swapChain;
        std::shared_ptr<SwapChain> m_oldSwapChain;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;
        size_t m_currentFrame = 0;
    };
}