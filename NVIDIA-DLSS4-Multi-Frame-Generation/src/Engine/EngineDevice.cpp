#include "EngineDevice.h"

#include <iostream>
#include <cstring>
#include <set>
#include <unordered_set>

namespace Engine
{
    // Local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT _messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
        void* _pUserData) 
    {
        std::cerr << "validation layer: " << _pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance _instance,
        const VkDebugUtilsMessengerCreateInfoEXT* _pCreateInfo,
        const VkAllocationCallbacks* _pAllocator,
        VkDebugUtilsMessengerEXT* _pDebugMessenger) 
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( _instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) 
            return func(_instance, _pCreateInfo, _pAllocator, _pDebugMessenger);
        else 
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance _instance,
        VkDebugUtilsMessengerEXT _debugMessenger,
        const VkAllocationCallbacks* _pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(_instance, _debugMessenger, _pAllocator);
    }

    // Class member functions
    EngineDevice::EngineDevice(std::weak_ptr<EngineWindow> _window) : m_window(_window)
    {
        createInstance(); // Initializes Vulkan library and our conection to it
        setupDebugMessenger(); // Set up debug messenger for validation layers to check for errors on unreleased builds
        createSurface(); // Create a surface for rendering making use of GLFW
        pickPhysicalDevice(); // Pick a suitable physical device (GPU) for rendering with Vulkan
        createLogicalDevice(); // Create a logical device to interface with the physical device
        createCommandPool(); // Create a command pool for managing command buffers
    }
    EngineDevice::~EngineDevice()
    {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroyDevice(m_device, nullptr);

        if (m_enableValidationLayers)
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }

    void EngineDevice::createInstance()
    {
        if (m_enableValidationLayers && !checkValidationLayerSupport())
            throw std::runtime_error("validation layers requested, but not available!");

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "LittleVulkanEngine App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (m_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance!");

        hasGflwRequiredInstanceExtensions();
    }

    void EngineDevice::pickPhysicalDevice() 
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0) 
            throw std::runtime_error("failed to find GPUs with Vulkan support!");

        std::cout << "Device count: " << deviceCount << std::endl;
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) 
        {
            if (isDeviceSuitable(device)) 
            {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) 
            throw std::runtime_error("failed to find a suitable GPU!");

        vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
        std::cout << "physical device: " << properties.deviceName << std::endl;
    }

    void EngineDevice::createLogicalDevice() 
    {
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.m_graphicsFamily, indices.m_presentFamily };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) 
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

        // Might not really be necessary anymore because device specific validation layers have been deprecated
        if (m_enableValidationLayers) 
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else 
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        vkGetDeviceQueue(m_device, indices.m_graphicsFamily, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices.m_presentFamily, 0, &m_presentQueue);
    }

    void EngineDevice::createCommandPool() 
    {
        QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) 
            throw std::runtime_error("failed to create command pool!");
    }

    void EngineDevice::createSurface() { m_window.lock()->createWindowSurface(m_instance, &m_surface); }

    bool EngineDevice::isDeviceSuitable(VkPhysicalDevice _device) 
    {
        QueueFamilyIndices indices = findQueueFamilies(_device);

        bool extensionsSupported = checkDeviceExtensionSupport(_device);

        bool swapChainAdequate = false;
        if (extensionsSupported) 
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device);
            swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(_device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    void EngineDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo) 
    {
        _createInfo = {};
        _createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        _createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        _createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        _createInfo.pfnUserCallback = debugCallback;
        _createInfo.pUserData = nullptr;
    }

    void EngineDevice::setupDebugMessenger() 
    {
        if (!m_enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) 
            throw std::runtime_error("failed to set up debug messenger!");
    }

    bool EngineDevice::checkValidationLayerSupport() 
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : m_validationLayers) 
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) 
            {
                if (strcmp(layerName, layerProperties.layerName) == 0) 
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) 
                return false;
        }
        return true;
    }

    std::vector<const char*> EngineDevice::getRequiredExtensions() 
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enableValidationLayers) 
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    void EngineDevice::hasGflwRequiredInstanceExtensions() 
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "available extensions:" << std::endl;
        std::unordered_set<std::string> available;
        for (const auto& extension : extensions) 
        {
            std::cout << "\t" << extension.extensionName << std::endl;
            available.insert(extension.extensionName);
        }

        std::cout << "required extensions:" << std::endl;
        auto requiredExtensions = getRequiredExtensions();
        for (const auto& required : requiredExtensions) 
        {
            std::cout << "\t" << required << std::endl;
            if (available.find(required) == available.end())
                throw std::runtime_error("Missing required glfw extension");
        }
    }

    bool EngineDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) 
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extensionCount,
            availableExtensions.data());

        std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

        for (const auto& extension : availableExtensions) 
            requiredExtensions.erase(extension.extensionName);

        return requiredExtensions.empty();
    }

    QueueFamilyIndices EngineDevice::findQueueFamilies(VkPhysicalDevice _device) 
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) 
        {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                indices.m_graphicsFamily = i;
                indices.m_graphicsFamilyHasValue = true;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, m_surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport) 
            {
                indices.m_presentFamily = i;
                indices.m_presentFamilyHasValue = true;
            }

            if (indices.isComplete()) 
                break;

            i++;
        }

        return indices;
    }

    SwapChainSupportDetails EngineDevice::querySwapChainSupport(VkPhysicalDevice device) 
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.m_capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        if (formatCount != 0) 
        {
            details.m_formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.m_formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) 
        {
            details.m_presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                m_surface,
                &presentModeCount,
                details.m_presentModes.data());
        }
        return details;
    }

    VkFormat EngineDevice::findSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features) 
    {
        for (VkFormat format : _candidates) 
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

            if (_tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & _features) == _features)
                return format;
            
            else if (_tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & _features) == _features) 
                return format;
        }
        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t EngineDevice::findMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties) 
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
        {
            if ((_typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties) 
                return i;
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void EngineDevice::createBuffer(
        VkDeviceSize _size,
        VkBufferUsageFlags _usage,
        VkMemoryPropertyFlags _properties,
        VkBuffer& _buffer,
        VkDeviceMemory& _bufferMemory) 
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = _size;
        bufferInfo.usage = _usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) 
            throw std::runtime_error("failed to create vertex buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, _buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) 
            throw std::runtime_error("failed to allocate vertex buffer memory!");

        vkBindBufferMemory(m_device, _buffer, _bufferMemory, 0);
    }

    VkCommandBuffer EngineDevice::beginSingleTimeCommands() 
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void EngineDevice::endSingleTimeCommands(VkCommandBuffer _commandBuffer) 
    {
        vkEndCommandBuffer(_commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffer;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool, 1, &_commandBuffer);
    }

    void EngineDevice::copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size) 
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = _size;
        vkCmdCopyBuffer(commandBuffer, _srcBuffer, _dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void EngineDevice::copyBufferToImage(VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height, uint32_t _layerCount) 
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = _layerCount;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { _width, _height, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            _buffer,
            _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
        endSingleTimeCommands(commandBuffer);
    }

    void EngineDevice::createImageWithInfo(
        const VkImageCreateInfo& _imageInfo,
        VkMemoryPropertyFlags _properties,
        VkImage& _image,
        VkDeviceMemory& _imageMemory) 
    {
        if (vkCreateImage(m_device, &_imageInfo, nullptr, &_image) != VK_SUCCESS)
            throw std::runtime_error("failed to create image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, _image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate image memory!");

        if (vkBindImageMemory(m_device, _image, _imageMemory, 0) != VK_SUCCESS)
            throw std::runtime_error("failed to bind image memory!");
    }

    void EngineDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
    {
        // Uses an image memory barrier transition image layouts and transfer queue
        // family ownership when VK_SHARING_MODE_EXCLUSIVE is used. There is an
        // equivalent buffer memory barrier to do this for buffers
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) 
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else 
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else 
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }
}