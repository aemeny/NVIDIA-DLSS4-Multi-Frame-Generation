#include "SwapChain.h"

#include "FrameGenerationHandler.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <thread>

namespace Engine
{
    SwapChain::SwapChain(EngineDevice& _deviceRef, VkExtent2D _extent, SlVkProxies& _slProxies)
        : m_device(_deviceRef), m_windowExtent(_extent), m_slProxies(_slProxies)
    {
        init();
    }

    SwapChain::SwapChain(EngineDevice& _deviceRef, VkExtent2D _windowExtent, std::shared_ptr<SwapChain> _previous, SlVkProxies& _slProxies)
        : m_device(_deviceRef), m_windowExtent(_windowExtent), m_oldSwapChain(_previous), m_slProxies(_slProxies)
    {
        init();

        m_oldSwapChain = nullptr; // Clear the old swap chain after initialization
    }

    void SwapChain::init()
    {
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDepthResources();
        createMotionVectorResources();
        createFramebuffers();
        createSyncObjects();
    }

    SwapChain::~SwapChain() 
    {
        // Destroy swap chain resources
        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_device.device(), imageView, nullptr);
        }
        m_swapChainImageViews.clear();

        if (m_swapChain != nullptr) 
        {
            //vkDestroySwapchainKHR(m_device.device(), m_swapChain, nullptr);
            m_slProxies.DestroySwapchainKHR(m_device.device(), m_swapChain, nullptr);
            m_swapChain = nullptr;
        }

        // Destroy depth resources
        for (int i = 0; i < m_depthImages.size(); i++) 
        {
            vkDestroyImageView(m_device.device(), m_depthImageViews[i], nullptr);
            vkDestroyImage(m_device.device(), m_depthImages[i], nullptr);
            vkFreeMemory(m_device.device(), m_depthImageMemories[i], nullptr);
        }

        // Destroy MV resources
        for (size_t i = 0; i < m_motionVectorImageViews.size(); ++i)
        {
            vkDestroyImageView(m_device.device(), m_motionVectorImageViews[i], nullptr);
        }
        for (size_t i = 0; i < m_motionVectorImages.size(); ++i)
        {
            vkDestroyImage(m_device.device(), m_motionVectorImages[i], nullptr);
            vkFreeMemory(m_device.device(), m_motionVectorImageMemories[i], nullptr);
        }

        // Destroy frame buffers
        for (auto framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_device.device(), framebuffer, nullptr);
        }
        
        // Destroy renderPass
        vkDestroyRenderPass(m_device.device(), m_renderPass, nullptr);

        // cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
        {
            vkDestroySemaphore(m_device.device(), m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device.device(), m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device.device(), m_inFlightFences[i], nullptr);
        }
    }

    VkResult SwapChain::acquireNextImage(uint32_t* _imageIndex) 
    {
        vkWaitForFences(
            m_device.device(),
            1,
            &m_inFlightFences[m_currentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max()
        );

        //VkResult result = vkAcquireNextImageKHR(
        //    m_device.device(),
        //    m_swapChain,
        //    std::numeric_limits<uint64_t>::max(),
        //    m_imageAvailableSemaphores[m_currentFrame],  // must be a not signaled semaphore
        //    VK_NULL_HANDLE,
        //    _imageIndex
        //);

        VkResult result = m_slProxies.AcquireNextImageKHR(
            m_device.device(),
            m_swapChain,
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailableSemaphores[m_currentFrame],  // must be a not signaled semaphore
            VK_NULL_HANDLE,
            _imageIndex
        );

        return result;
    }

    VkResult SwapChain::submitCommandBuffers(const VkCommandBuffer* _buffers, uint32_t* _imageIndex, FrameGenerationHandler* _frameGen)
    {
        if (m_imagesInFlight[*_imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_device.device(), 1, &m_imagesInFlight[*_imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[*_imageIndex] = m_inFlightFences[m_currentFrame];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = _buffers;
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[*_imageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(m_device.device(), 1, &m_inFlightFences[m_currentFrame]);

        if (_frameGen) 
            _frameGen->reflexRenderSubmitStart(_frameGen->getFrameToken());

        if (vkQueueSubmit(m_device.graphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) 
            throw std::runtime_error("failed to submit draw command buffer!");

        if (_frameGen)
            _frameGen->reflexRenderSubmitEnd(_frameGen->getFrameToken());

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = _imageIndex;

        if (_frameGen)
        {
            _frameGen->reflexPresentStart(_frameGen->getFrameToken());
        }

       // auto result = vkQueuePresentKHR(m_device.presentQueue(), &presentInfo);
        auto result = m_slProxies.QueuePresentKHR(m_device.presentQueue(), &presentInfo);

        if (_frameGen)
        {
            _frameGen->reflexPresentEnd(_frameGen->getFrameToken());
            if (result == VK_SUCCESS) _frameGen->markPresented();
            _frameGen->updateState();
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }

    void SwapChain::createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = m_device.getSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.m_formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.m_presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.m_capabilities);

        uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
        if (swapChainSupport.m_capabilities.maxImageCount > 0 &&  imageCount > swapChainSupport.m_capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.m_capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_device.surface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = m_device.findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily, indices.m_presentFamily };

        if (indices.m_graphicsFamily != indices.m_presentFamily) 
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else 
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      
            createInfo.pQueueFamilyIndices = nullptr;  
        }

        createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_swapChain;

        //if (vkCreateSwapchainKHR(m_device.device(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) 
        if (m_slProxies.CreateSwapchainKHR(m_device.device(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swap chain!");

        /* Only specified a minimum number of images in the swap chain, so the implementation is allowed 
           to create a swap chain with more. First query the final number of images with vkGetSwapChainImagesKHR, 
           then resize the container and finally call it again to retrieve the handles. */
        //vkGetSwapchainImagesKHR(m_device.device(), m_swapChain, &imageCount, nullptr);
        m_slProxies.GetSwapchainImagesKHR(m_device.device(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        m_slProxies.GetSwapchainImagesKHR(m_device.device(), m_swapChain, &imageCount, m_swapChainImages.data());
        //vkGetSwapchainImagesKHR(m_device.device(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void SwapChain::createImageViews() 
    {
        m_swapChainImageViews.resize(m_swapChainImages.size());
        for (size_t i = 0; i < m_swapChainImages.size(); i++) 
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) 
                throw std::runtime_error("Failed to create texture image view!");
        }
    }

    void SwapChain::createRenderPass() 
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription mvAttachment = {}; // Motion Vector
        mvAttachment.format = VK_FORMAT_R16G16_SFLOAT;
        mvAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        mvAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        mvAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        mvAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        mvAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        mvAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        mvAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 2;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference mvAttachmentRef{};
        mvAttachmentRef.attachment = 1;
        mvAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colourAttachment = {};
        colourAttachment.format = getSwapChainImageFormat();
        colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colourAttachmentRef = {};
        colourAttachmentRef.attachment = 0;
        colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        std::array<VkAttachmentReference, 2> colourRefs = { colourAttachmentRef, mvAttachmentRef };
        subpass.colorAttachmentCount = static_cast<uint32_t>(colourRefs.size());
        subpass.pColorAttachments = colourRefs.data();

        VkSubpassDependency dependency = {};
        dependency.dstSubpass = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colourAttachment, mvAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device.device(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) 
            throw std::runtime_error("Failed to create render pass!");
    }

    void SwapChain::createFramebuffers() 
    {
        m_swapChainFramebuffers.resize(imageCount());
        for (size_t i = 0; i < imageCount(); i++) 
        {
            std::array<VkImageView, 3> attachments = { m_swapChainImageViews[i], m_motionVectorImageViews[i], m_depthImageViews[i] };

            VkExtent2D swapChainExtent = getSwapChainExtent();
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device.device(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    void SwapChain::createDepthResources() 
    {
        VkFormat depthFormat = findDepthFormat();
        m_swapChainDepthFormat = depthFormat;
        VkExtent2D swapChainExtent = getSwapChainExtent();

        m_depthImages.resize(imageCount());
        m_depthImageMemories.resize(imageCount());
        m_depthImageViews.resize(imageCount());

        for (int i = 0; i < m_depthImages.size(); i++) 
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = swapChainExtent.width;
            imageInfo.extent.height = swapChainExtent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = 0;

            m_device.createImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_depthImages[i],
                m_depthImageMemories[i]
            );

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_depthImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_depthImageViews[i]) != VK_SUCCESS) 
                throw std::runtime_error("Failed to create texture image view!");
        }
    }

    void SwapChain::createMotionVectorResources()
    {
        m_motionVectorImages.resize(imageCount());
        m_motionVectorImageMemories.resize(imageCount());
        m_motionVectorImageViews.resize(imageCount());

        VkExtent2D extent = getSwapChainExtent();

        for (size_t i = 0; i < imageCount(); i++)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = { extent.width, extent.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R16G16_SFLOAT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            m_device.createImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_motionVectorImages[i],
                m_motionVectorImageMemories[i]);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_motionVectorImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R16G16_SFLOAT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_motionVectorImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create MV image view!");
        }
    }

    void SwapChain::createSyncObjects()
    {
        uint32_t imageCount = static_cast<uint32_t>(m_swapChainImages.size());
        
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(imageCount);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device.device(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device.device(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create sync objects for a frame!");
            }
        }

        for (uint32_t i = 0; i < imageCount; i++)
        {
            if (vkCreateSemaphore(m_device.device(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create render-finished semaphore for swapchain image!");
            }
        }
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats) 
    {
        // Prefer UNORM for DLSS-G
        for (const auto& format  : _availableFormats) 
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }
        for (const auto& format : _availableFormats)
        {
            if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }

        return _availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes) 
    {
        // Comment out both modes for FIFO:
        /* +VSYNC BOUND, +GOOD FOR WEAKER DEVICES(MOLBILE), +ALWAYS SUPPORTED, -BAD LATENCY */

        /* +LOW LATENCY, -NOT ALWAYS SUPPORTED, -HIGH POWER USAGE */
        //for (const auto& availablePresentMode : _availablePresentModes) 
        //{
        //    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
        //    {
        //        std::cout << "Present mode: Mailbox" << std::endl;
        //        return availablePresentMode;
        //    }
        //}

        /* +BEST FOR PERFORMANCE TESTING, +LOW LATENCY, -TEARING, -HIGH POWER USAGE */
         for (const auto &availablePresentMode : _availablePresentModes) 
         {
           if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) 
           {
             std::cout << "Present mode: Immediate" << std::endl;
             return availablePresentMode;
           }
         }

        std::cout << "Present mode: V-Sync" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities) 
    {
        if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
        {
            return _capabilities.currentExtent;
        }
        else 
        {
            VkExtent2D actualExtent = m_windowExtent;

            actualExtent.width = std::max(_capabilities.minImageExtent.width,
                                 std::min(_capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(_capabilities.minImageExtent.height,
                                  std::min(_capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    VkFormat SwapChain::findDepthFormat() 
    {
        return m_device.findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
}