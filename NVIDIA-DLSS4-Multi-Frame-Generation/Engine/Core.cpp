#include "Core.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <iostream>

namespace Engine
{
    struct SimplePushConstantData
    {
        glm::mat2 m_transform{1.0f};
        glm::vec2 m_offset;
        alignas(16) glm::vec3 m_colour;
    };


    Core::Core(std::shared_ptr<EngineWindow> _window)
        : m_window(_window) 
    {
        loadGameObjects();
        createPipelineLayout();
        recreateSwapChain();
        std::cout << "Max Push Constant Size: " << m_device.properties.limits.maxPushConstantsSize << std::endl;
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

    void Core::loadGameObjects()
    {
        std::vector<Model::Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        std::shared_ptr<Model> m_model = std::make_shared<Model>(m_device, vertices);

        GameObject triangle = GameObject::createGameObject();
        triangle.m_model = m_model;
        triangle.m_colour = { 0.1f, 0.8f, 0.1f };
        triangle.m_transform2D.m_translation.x = 0.2f;
        triangle.m_transform2D.m_scale = { 2.0f, 0.5f };
        triangle.m_transform2D.m_rotation = 0.25f * glm::two_pi<float>();

        m_gameObjects.push_back(std::move(triangle));
    }

    void Core::createPipelineLayout()
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
    }

    void Core::createPipeline()
    {
        assert(m_swapChain != nullptr && "Cannot create graphics pipeline: No swap chain provided");
        assert(m_pipelineLayout != nullptr && "Cannot create graphics pipeline: No pipelineLayout provided");

        PipelineConfigInfo pipelineConfig = {};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
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

    void Core::freeCommandBuffers()
    {
        vkFreeCommandBuffers(m_device.device(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
        m_commandBuffers.resize(0);
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
        clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f }; // Clear color
        clearValues[1].depthStencil = { 1.0f, 0 }; // Clear depth

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport and scissor dynamically
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(m_swapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor = { {0, 0}, m_swapChain->getSwapChainExtent()};
        vkCmdSetViewport(m_commandBuffers[_imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(m_commandBuffers[_imageIndex], 0, 1, &scissor);

        renderGameObjects(m_commandBuffers[_imageIndex]);

        vkCmdEndRenderPass(m_commandBuffers[_imageIndex]);
        if (vkEndCommandBuffer(m_commandBuffers[_imageIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }

    void Core::renderGameObjects(VkCommandBuffer _commandBuffer)
    {
        m_pipeline->bind(_commandBuffer);

        for (GameObject& obj : m_gameObjects)
        {
            obj.m_transform2D.m_rotation = glm::mod(obj.m_transform2D.m_rotation + 0.001f, glm::two_pi<float>());

            SimplePushConstantData push = {};
            push.m_offset = obj.m_transform2D.m_translation;
            push.m_colour = obj.m_colour;
            push.m_transform = obj.m_transform2D.mat2();

            vkCmdPushConstants(_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
            
            obj.m_model->bind(_commandBuffer);
            obj.m_model->draw(_commandBuffer);
        }
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
        
        if (m_swapChain == nullptr)
        {
            m_swapChain = std::make_unique<SwapChain>(m_device, extend);
        }
        else
        {
            m_swapChain = std::make_unique<SwapChain>(m_device, extend, std::move(m_swapChain));
            if (m_swapChain->imageCount() != m_commandBuffers.size())
            {
                freeCommandBuffers();
                createCommandBuffers();
            }
        }

        // possible future improvement: If render pass is compatable do nothing else
        createPipeline();
    }
}