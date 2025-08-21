#include "SlVkProxies.h"
#if defined(_WIN32)
#  ifndef NOMINMAX
#  define NOMINMAX
#  endif
#  include <Windows.h>
#endif

namespace Engine
{
    SlVkProxies g_slvk;
    void SlVkProxies::initializeModule(bool _enable)
    {
        m_enabled = _enable;

#if defined(_WIN32)
        if (!m_enabled) return;

        HMODULE h = LoadLibraryA("sl.interposer.dll");
        if (!h) { m_enabled = false; return; }

        m_vkGetInstanceProcAddrProxy = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(h, "vkGetInstanceProcAddr"));
        m_vkGetDeviceProcAddrProxy = reinterpret_cast<PFN_vkGetDeviceProcAddr>(GetProcAddress(h, "vkGetDeviceProcAddr"));

        if (!m_vkGetInstanceProcAddrProxy || !m_vkGetDeviceProcAddrProxy)
        {
            m_enabled = false;
            return;
        }
#else
        m_enabled = false;
#endif
    }

    void SlVkProxies::resolve(VkInstance _instance, VkDevice _device)
    {
        if (!m_enabled) return;

        m_vkCreateSwapchainKHRProxy = reinterpret_cast<PFN_vkCreateSwapchainKHR>(m_vkGetDeviceProcAddrProxy(_device, "vkCreateSwapchainKHR"));
        m_vkDestroySwapchainKHRProxy = reinterpret_cast<PFN_vkDestroySwapchainKHR>(m_vkGetDeviceProcAddrProxy(_device, "vkDestroySwapchainKHR"));
        m_vkGetSwapchainImagesKHRProxy = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(m_vkGetDeviceProcAddrProxy(_device, "vkGetSwapchainImagesKHR"));
        m_vkAcquireNextImageKHRProxy = reinterpret_cast<PFN_vkAcquireNextImageKHR>(m_vkGetDeviceProcAddrProxy(_device, "vkAcquireNextImageKHR"));
        m_vkQueuePresentKHRProxy = reinterpret_cast<PFN_vkQueuePresentKHR>(m_vkGetDeviceProcAddrProxy(_device, "vkQueuePresentKHR"));
        m_vkGetDeviceQueueProxy = reinterpret_cast<PFN_vkGetDeviceQueue>(m_vkGetDeviceProcAddrProxy(_device, "vkGetDeviceQueue"));

        m_vkCreateDeviceProxy = reinterpret_cast<PFN_vkCreateDevice>(m_vkGetInstanceProcAddrProxy(_instance, "vkCreateDevice"));
        m_vkCreateInstanceProxy = reinterpret_cast<PFN_vkCreateInstance>(m_vkGetInstanceProcAddrProxy(_instance, "vkCreateInstance"));
    }

    VkResult SlVkProxies::CreateSwapchainKHR(VkDevice _device,
        const VkSwapchainCreateInfoKHR* _createInfo,
        const VkAllocationCallbacks* _alloc,
        VkSwapchainKHR* _swapchain)
    {
        if (m_enabled && m_vkCreateSwapchainKHRProxy) return m_vkCreateSwapchainKHRProxy(_device, _createInfo, _alloc, _swapchain);
        return vkCreateSwapchainKHR(_device, _createInfo, _alloc, _swapchain);
    }

    void SlVkProxies::DestroySwapchainKHR(VkDevice _device, VkSwapchainKHR _swapchain, const VkAllocationCallbacks* _alloc)
    {
        if (m_enabled && m_vkDestroySwapchainKHRProxy) { m_vkDestroySwapchainKHRProxy(_device, _swapchain, _alloc); return; }
        vkDestroySwapchainKHR(_device, _swapchain, _alloc);
    }

    VkResult SlVkProxies::GetSwapchainImagesKHR(VkDevice _device,
        VkSwapchainKHR _swapchain,
        uint32_t* _count,
        VkImage* _images)
    {
        if (m_enabled && m_vkGetSwapchainImagesKHRProxy) return m_vkGetSwapchainImagesKHRProxy(_device, _swapchain, _count, _images);
        return vkGetSwapchainImagesKHR(_device, _swapchain, _count, _images);
    }

    VkResult SlVkProxies::AcquireNextImageKHR(VkDevice _device,
        VkSwapchainKHR _swapchain,
        uint64_t _timeout,
        VkSemaphore _semaphore,
        VkFence _fence,
        uint32_t* _imageIndex)
    {
        if (m_enabled && m_vkAcquireNextImageKHRProxy) return m_vkAcquireNextImageKHRProxy(_device, _swapchain, _timeout, _semaphore, _fence, _imageIndex);
        return vkAcquireNextImageKHR(_device, _swapchain, _timeout, _semaphore, _fence, _imageIndex);
    }

    VkResult SlVkProxies::QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR* _presentInfo)
    {
        if (m_enabled && m_vkQueuePresentKHRProxy) return m_vkQueuePresentKHRProxy(_queue, _presentInfo);
        return vkQueuePresentKHR(_queue, _presentInfo);
    }

    void SlVkProxies::GetDeviceQueue(VkDevice _device,
        uint32_t _queueFamilyIndex,
        uint32_t _queueIndex,
        VkQueue* _pQueue)
    {
        if (m_enabled && m_vkGetDeviceQueueProxy) { m_vkGetDeviceQueueProxy(_device, _queueFamilyIndex, _queueIndex, _pQueue); return; }
        vkGetDeviceQueue(_device, _queueFamilyIndex, _queueIndex, _pQueue);
    }
}