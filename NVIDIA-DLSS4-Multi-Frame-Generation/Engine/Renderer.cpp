#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <iostream>

namespace Engine
{
    Renderer::Renderer(std::weak_ptr<EngineWindow> _window, EngineDevice& _device)
        : m_window(_window), m_device(_device)
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

        auto result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.lock()->hasWindowResized())
        {
            m_window.lock()->resetWindowResizedFlag();
            recreateSwapChain();
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

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f }; // Clear color
        clearValues[1].depthStencil = { 1.0f, 0 }; // Clear depth

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
            m_swapChain = std::make_unique<SwapChain>(m_device, extend);
        }
        else
        {
            std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
            m_swapChain = std::make_unique<SwapChain>(m_device, extend, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*m_swapChain.get()))
                throw std::runtime_error("Swap chain image or depth format has changed!");
        }

        // possible future improvement: If render pass is compatable do nothing else
        // createPipeline(); TODO
    }
}