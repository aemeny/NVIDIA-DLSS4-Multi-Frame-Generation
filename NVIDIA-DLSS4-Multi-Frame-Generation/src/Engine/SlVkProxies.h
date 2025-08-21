#pragma once

#include <vulkan/vulkan.h>

namespace Engine
{
    struct SlVkProxies
    {
        bool m_enabled = false;
        PFN_vkGetDeviceProcAddr m_vkGetDeviceProcAddrProxy = nullptr;
        PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddrProxy = nullptr;

        PFN_vkCreateSwapchainKHR m_vkCreateSwapchainKHRProxy = nullptr;
        PFN_vkDestroySwapchainKHR m_vkDestroySwapchainKHRProxy = nullptr;
        PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHRProxy = nullptr;
        PFN_vkAcquireNextImageKHR m_vkAcquireNextImageKHRProxy = nullptr;
        PFN_vkQueuePresentKHR m_vkQueuePresentKHRProxy = nullptr;
        PFN_vkGetDeviceQueue m_vkGetDeviceQueueProxy = nullptr;

        PFN_vkCreateDevice m_vkCreateDeviceProxy = nullptr;
        PFN_vkCreateInstance m_vkCreateInstanceProxy = nullptr;

        void initializeModule(bool _enable);

        void resolve(VkInstance _instance, VkDevice _device);

        VkResult CreateSwapchainKHR(VkDevice _device,
            const VkSwapchainCreateInfoKHR* _createInfo,
            const VkAllocationCallbacks* _alloc,
            VkSwapchainKHR* _swapchain
        );

        void DestroySwapchainKHR(VkDevice _device,
            VkSwapchainKHR _swapchain,
            const VkAllocationCallbacks* _alloc
        );

        VkResult GetSwapchainImagesKHR(VkDevice _device,
            VkSwapchainKHR _swapchain,
            uint32_t* _count,
            VkImage* _images
        );

        VkResult AcquireNextImageKHR(VkDevice _device,
            VkSwapchainKHR _swapchain,
            uint64_t _timeout,
            VkSemaphore _semaphore,
            VkFence _fence,
            uint32_t* _imageIndex
        );

        void GetDeviceQueue(VkDevice _device,
            uint32_t _queueFamilyIndex,
            uint32_t _queueIndex,
            VkQueue* _pQueue);

        VkResult QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR* _presentInfo);
    };
    extern SlVkProxies g_slvk;
}