#include "FrameGenerationHandler.h"

#include <vulkan/vulkan.h>
#include <Streamline/sl_consts.h>
#include <Streamline/sl_dlss_g.h>
#include <Streamline/sl_helpers_vk.h>

#include <stdexcept>

namespace Engine
{
    FrameGenerationHandler::FrameGenerationHandler() 
    {
        sl::Preferences pref;
        pref.showConsole = true;
        pref.logLevel = sl::LogLevel::eDefault;
        pref.pathsToPlugins = {}; // Path to streamline plugins
        pref.numPathsToPlugins = 0; 
        pref.pathToLogsAndData = {}; // Path to logs in a file
        pref.allocateCallback = {}; // Resource allocation callback
        pref.applicationId = 0; // Application ID when using NGX components
        pref.engine = sl::EngineType::eCustom;
        pref.engineVersion = "1.0.0"; // Engine version
        pref.renderAPI = sl::RenderAPI::eVulkan; // Specify Vulkan as the render API
        const sl::Feature features[] = { sl::kFeatureDLSS_G };
        pref.featuresToLoad = features;
        pref.numFeaturesToLoad = _countof(features);
        

        if (SL_FAILED(res, slInit(pref)))
        {
            if (res == sl::Result::eErrorDriverOutOfDate)
            {
                throw std::runtime_error("Streamline initialization failed: Driver is out of date.");
            }
            else if (res == sl::Result::eErrorOSOutOfDate)
            {
                throw std::runtime_error("Streamline initialization failed: OS is out of date.");
            }
            else
            {
                throw std::runtime_error("Streamline initialization failed with error code: " + std::to_string(static_cast<int>(res)));
            }
        }
    };

    FrameGenerationHandler::~FrameGenerationHandler() {}

    void FrameGenerationHandler::shutDownStreamline()
    {
        if (SL_FAILED(res, slShutdown()))
        {
            throw std::runtime_error("Streamline shutdown failed with error code: " + std::to_string(static_cast<int>(res)));
        }
    }

    void FrameGenerationHandler::initializeStreamline(EngineDevice& _device)
    {
        sl::VulkanInfo vkInfo{};

        vkInfo.instance = _device.instance();
        vkInfo.physicalDevice = _device.physicalDevice();
        vkInfo.device = _device.device();

        QueueFamilyIndices indices = _device.findPhysicalQueueFamilies();
        vkInfo.graphicsQueueFamily = indices.m_graphicsFamily;
        vkInfo.graphicsQueueIndex = 0; 
        vkInfo.computeQueueFamily = indices.m_graphicsFamily; // Assuming compute and graphics share the same family
        vkInfo.computeQueueIndex = 0; 
        vkInfo.opticalFlowQueueFamily = indices.m_graphicsFamily; // Assuming optical flow shares the same family
        vkInfo.opticalFlowQueueIndex = 0;
        vkInfo.useNativeOpticalFlowMode = false; 

        vkInfo.computeQueueCreateFlags = 0;
        vkInfo.graphicsQueueCreateFlags = 0;
        vkInfo.opticalFlowQueueCreateFlags = 0;      

        if (SL_FAILED(res, slSetVulkanInfo(vkInfo)))
        {
            throw std::runtime_error("Streamline Vulkan Info failed with error code: " + std::to_string(static_cast<int>(res)));
        }
    }
}