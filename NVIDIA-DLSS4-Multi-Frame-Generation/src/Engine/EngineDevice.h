#pragma once

#include "Window.h"

#include <vector>
#include <memory>

namespace Engine
{
    struct SwapChainSupportDetails 
    {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_presentModes;
    };

    struct QueueFamilyIndices 
    {
        uint32_t m_graphicsFamily;
        uint32_t m_presentFamily;
        bool m_graphicsFamilyHasValue = false;
        bool m_presentFamilyHasValue = false;
        bool isComplete() { return m_graphicsFamilyHasValue && m_presentFamilyHasValue; }
    };

    struct EngineDevice
    {
#ifdef NDEBUG
        const bool m_enableValidationLayers = false;
#else
        const bool m_enableValidationLayers = true;
#endif

        EngineDevice(std::weak_ptr<EngineWindow> _window);
        ~EngineDevice();

        // Not copyable or movable
        EngineDevice(const EngineDevice&) = delete;
        void operator=(const EngineDevice&) = delete;
        EngineDevice(EngineDevice&&) = delete;
        EngineDevice& operator=(EngineDevice&&) = delete;

        VkCommandPool getCommandPool() { return m_commandPool; }
        VkDevice device() { return m_device; }
        VkSurfaceKHR surface() { return m_surface; }
        VkQueue graphicsQueue() { return m_graphicsQueue; }
        VkQueue presentQueue() { return m_presentQueue; }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); }
        uint32_t findMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties);
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); }
        VkFormat findSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features);
        
        // Buffer Helper Functions
        void createBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferMemory);
        void copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size);
        void copyBufferToImage(VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height, uint32_t _layerCount);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer _commandBuffer);

        void createImageWithInfo(const VkImageCreateInfo& _imageCreateInfo, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory);
    
        VkPhysicalDeviceProperties properties;

    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();

        // Helper Functions
        bool isDeviceSuitable(VkPhysicalDevice _device);
        std::vector<const char*> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
        void hasGflwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice _device);

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        std::weak_ptr<EngineWindow> m_window;
        VkCommandPool m_commandPool;

        VkDevice m_device;
        VkSurfaceKHR m_surface;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

        const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
}