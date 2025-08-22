#include "Renderer.h"

#include "FrameGenerationHandler.h"

#include <stdexcept>
#include <array>
#include <iostream>

namespace Engine
{
    Renderer::Renderer(std::weak_ptr<EngineWindow> _window, EngineDevice& _device, SlVkProxies& _slProxies)
        : m_window(_window), m_device(_device), m_slProxies(_slProxies)
    {
        std::cout << "Max Push Constant Size: " << m_device.properties.limits.maxPushConstantsSize << std::endl;
        recreateSwapChain();
        createCommandBuffers();
    }

    Renderer::~Renderer()
    {
        freeCommandBuffers();
    }

    VkCommandBuffer Renderer::beginFrame()
    {
        assert(!m_isFrameStarted && "Cannot call beginFrame while a frame is already in progress!");

        auto result = m_swapChain->acquireNextImage(&m_currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image!");

        m_isFrameStarted = true;

        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");

        return commandBuffer;
    }

    void Renderer::endFrame()
    {
        assert(m_isFrameStarted && "Cannot call endFrame while no frame is in progress!");

        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");

        if (m_frameGen)
        {
            m_frameGen->tagResources(
                m_swapChain->getDepthImage(m_currentImageIndex),
                m_swapChain->getDepthImageView(m_currentImageIndex),
                m_swapChain->getDepthImageMemory(m_currentImageIndex),

                m_swapChain->getMotionVectorImage(m_currentImageIndex),
                m_swapChain->getMotionVectorImageView(m_currentImageIndex),
                m_swapChain->getMotionVectorImageMemory(m_currentImageIndex),

                m_swapChain->getSwapChainImage(m_currentImageIndex),
                m_swapChain->getSwapChainImageView(m_currentImageIndex),
                VK_NULL_HANDLE,

                m_swapChain->getSwapChainExtent(),
                commandBuffer
            );

            //m_frameGen->evaluateFeature(commandBuffer);
        }

        auto result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex, m_frameGen);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.lock()->hasWindowResized())
        {
            m_window.lock()->resetWindowResizedFlag();
            recreateSwapChain();
            if (m_frameGen) m_frameGen->triggerReset(2);
        }
        else if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to submit command buffers!");

        m_isFrameStarted = false;
        m_currentFrameIndex = (m_currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::beginSwapChainRenderPass(VkCommandBuffer _commandBuffer)
    {
        assert(m_isFrameStarted && "Cannot begin render pass when frame is not in progress!");
        assert(_commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_swapChain->getRenderPass();
        renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(m_currentImageIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

        std::array<VkClearValue, 3> clearValues = {};
        clearValues[0].color = { 0.42f, 0.5f, 0.68f, 1.0f }; // Clear colour / Main colour
        clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Clear motion vector
        clearValues[2].depthStencil = { 1.0f, 0 }; // Clear depth

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport and scissor dynamically
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(m_swapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor = { {0, 0}, m_swapChain->getSwapChainExtent() };
        vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    }

    void Renderer::endSwapChainRenderPass(VkCommandBuffer _commandBuffer)
    {
        assert(m_isFrameStarted && "Cannot end render pass when frame is not in progress!");
        assert(_commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame!");

        vkCmdEndRenderPass(_commandBuffer);
    }

    void Renderer::pushSLCommonConstants(const glm::mat4& _viewMatrix, const glm::mat4& _projectionMatrix, 
        const glm::mat4& _prevViewMatrix, const glm::mat4& _prevProjectionMatrix, 
        float _nearZ, float _farZ, bool _depthInverted, 
        glm::vec2 _motionVecScale)
    {
        if (!m_frameGen) return;
        m_frameGen->setCommonConstants(
            _viewMatrix, _projectionMatrix, 
            _prevViewMatrix, _prevProjectionMatrix, 
            m_swapChain->getSwapChainExtent(), m_window.lock()->getExtent(), 
            _nearZ, _farZ, _depthInverted, 
            _motionVecScale);
    }

    void Renderer::createCommandBuffers()
    {
        m_commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_device.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(m_device.device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");
    }

    void Renderer::freeCommandBuffers()
    {
        vkFreeCommandBuffers(m_device.device(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
        m_commandBuffers.resize(0);
    }

    void Renderer::recreateSwapChain()
    {
        auto extend = m_window.lock()->getExtent();

        while (extend.width == 0 || extend.height == 0)
        {
            extend = m_window.lock()->getExtent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device.device()); // Wait for the device to finish all operations before recreating the swap chain

        if (m_swapChain == nullptr)
        {
            m_swapChain = std::make_unique<SwapChain>(m_device, extend, m_slProxies);
        }
        else
        {
            std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
            m_swapChain = std::make_unique<SwapChain>(m_device, extend, oldSwapChain, m_slProxies);

            if (!oldSwapChain->compareSwapFormats(*m_swapChain.get()))
                throw std::runtime_error("Swap chain image or depth format has changed!");
        }

        // possible future improvement: If render pass is compatable do nothing else
        // createPipeline(); TODO
    }
}