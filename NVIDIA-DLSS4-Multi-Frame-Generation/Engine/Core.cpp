#include "Core.h"

#include <stdexcept>
#include <array>

namespace Engine
{
    Core::Core(std::shared_ptr<EngineWindow> _window)
        : m_window(_window) 
    {
        loadModels();
        createPipelineLayout();
        recreateSwapChain();
        createCommandBuffers();
    }

    Core::~Core()
    {
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void Core::run()
    {
        m_terminateApplication = false;
        while (!m_window->shouldClose() && !m_terminateApplication)
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(m_device.device()); // Wait for the device to finish all operations before exiting
    }

    void Core::stop()
    {
        m_terminateApplication = true;
    }

    void Core::drawFrame()
    {
        uint32_t imageIndex;
        auto result = m_swapChain->acquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain(); 
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image!");

        recordCommandBuffers(imageIndex);

        result = m_swapChain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->hasWindowResized())
        {
            m_window->resetWindowResizedFlag();
            recreateSwapChain(); 
            return;
        }
        
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to submit command buffers!");
    }

    void Core::loadModels()
    {
        std::vector<Model::Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };

        m_model = std::make_unique<Model>(m_device, vertices);
    }

    void Core::createPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
    }

    void Core::createPipeline()
    {
        auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(m_swapChain->width(), m_swapChain->height());
        pipelineConfig.m_renderPass = m_swapChain->getRenderPass();
        pipelineConfig.m_pipelineLayout = m_pipelineLayout;
        m_pipeline = std::make_unique<Pipeline>(m_device, "Shaders/Unlit/Vertex.vert.spv", "Shaders/Unlit/Fragment.frag.spv", pipelineConfig);
    }

    void Core::createCommandBuffers()
    {
        m_commandBuffers.resize(m_swapChain->imageCount());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_device.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(m_device.device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");
    }

    void Core::recordCommandBuffers(int _imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_commandBuffers[_imageIndex], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_swapChain->getRenderPass();
        renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(_imageIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f }; // Clear color
        clearValues[1].depthStencil = { 1.0f, 0 }; // Clear depth

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        m_pipeline->bind(m_commandBuffers[_imageIndex]);
        m_model->bind(m_commandBuffers[_imageIndex]);
        m_model->draw(m_commandBuffers[_imageIndex]);

        vkCmdEndRenderPass(m_commandBuffers[_imageIndex]);
        if (vkEndCommandBuffer(m_commandBuffers[_imageIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }

    void Core::recreateSwapChain()
    {
        auto extend = m_window->getExtent();

        while (extend.width == 0 || extend.height == 0)
        {
            extend = m_window->getExtent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device.device()); // Wait for the device to finish all operations before recreating the swap chain
        m_swapChain = nullptr;
        m_swapChain = std::make_unique<SwapChain>(m_device, extend);
        createPipeline();
    }
}