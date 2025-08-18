#include "FrameGenerationHandler.h"

#include <vulkan/vulkan.h>
#include <Streamline/sl_consts.h>

#include <stdexcept>

namespace Engine
{
    FrameGenerationHandler::FrameGenerationHandler() 
    {
        m_preferences.showConsole = true;
        m_preferences.logLevel = sl::LogLevel::eDefault;
        m_preferences.pathsToPlugins = {}; // Path to streamline plugins
        m_preferences.numPathsToPlugins = 0;
        m_preferences.pathToLogsAndData = {}; // Path to logs in a file
        m_preferences.allocateCallback = {}; // Resource allocation callback

        //pref.applicationId = 0; // Application ID when using NGX components
        m_preferences.engine = sl::EngineType::eCustom;
        m_preferences.engineVersion = "1.0.0"; // Engine version
        m_preferences.projectId = { "a0f57b54-1daf-4934-90ae-c4035c19df04" };
        m_preferences.renderAPI = sl::RenderAPI::eVulkan; // Specify Vulkan as the render API

        const sl::Feature features[] = { sl::kFeatureDLSS_G, sl::kFeatureReflex };
        m_preferences.featuresToLoad = features;
        m_preferences.numFeaturesToLoad = _countof(features);
        
        

        if (SL_FAILED(res, slInit(m_preferences)))
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
        m_vkInfo.instance = _device.instance();
        m_vkInfo.physicalDevice = _device.physicalDevice();
        m_vkInfo.device = _device.device();

        QueueFamilyIndices indices = _device.findPhysicalQueueFamilies();
        m_vkInfo.graphicsQueueFamily = indices.m_graphicsFamily;
        m_vkInfo.graphicsQueueIndex = 0;
        m_vkInfo.computeQueueFamily = indices.m_graphicsFamily; // Assuming compute and graphics share the same family
        m_vkInfo.computeQueueIndex = 0;
        m_vkInfo.opticalFlowQueueFamily = indices.m_graphicsFamily; // Assuming optical flow shares the same family
        m_vkInfo.opticalFlowQueueIndex = 0;
        m_vkInfo.useNativeOpticalFlowMode = false;
        
        m_vkInfo.computeQueueCreateFlags = 0;
        m_vkInfo.graphicsQueueCreateFlags = 0;
        m_vkInfo.opticalFlowQueueCreateFlags = 0;

        if (SL_FAILED(res, slSetVulkanInfo(m_vkInfo)))
        {
            throw std::runtime_error("Streamline Vulkan Info failed with error code: " + std::to_string(static_cast<int>(res)));
        }

        sl::AdapterInfo adapter{};
        if (SL_FAILED(res, slIsFeatureSupported(sl::kFeatureDLSS_G, adapter)))
        {
            throw std::runtime_error("DLSS_G is not supported on this device. " + std::to_string(static_cast<int>(res)));
        }


        slSetFeatureLoaded(sl::kFeatureDLSS_G, true);
        
        m_DLSSGOptions.mode = sl::DLSSGMode::eOn;
        if (SL_FAILED(res, slDLSSGSetOptions(0, m_DLSSGOptions)))
        {
            throw std::runtime_error("DLSS set options failed: " + std::to_string(static_cast<int>(res)));
        }
    }

    bool FrameGenerationHandler::getFrameStats(FrameStats& _stats) const
    {
        sl::DLSSGState state{};
        slDLSSGGetState(0, state, &m_DLSSGOptions);
       
        _stats.m_totalPresentedFrameCount = state.numFramesActuallyPresented;
        _stats.m_isFrameGenerationEnabled = m_DLSSGOptions.mode != sl::DLSSGMode::eOff;

        return true;
    }
}