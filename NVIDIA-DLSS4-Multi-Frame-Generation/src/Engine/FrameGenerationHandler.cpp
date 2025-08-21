#include "FrameGenerationHandler.h"

#include <vulkan/vulkan.h>
#include <Streamline/sl_consts.h>
#include <Streamline/sl.h>
#include <Streamline/sl_reflex.h>
#include <glm/gtc/matrix_inverse.hpp>

#include <stdexcept>
#include <iostream>

namespace Engine
{
    FrameGenerationHandler::FrameGenerationHandler() {};

    FrameGenerationHandler::~FrameGenerationHandler() {}

    void FrameGenerationHandler::shutDownStreamline()
    {
        if (SL_FAILED(res, slShutdown()))
        {
            throw std::runtime_error("Streamline shutdown failed with error code: " + std::to_string(static_cast<int>(res)));
        }
    }

    void FrameGenerationHandler::generatePreferences()
    {
        m_preferences.showConsole = false;
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

        m_preferences.flags |= sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eUseManualHooking;
        const sl::Feature features[] = { sl::kFeatureDLSS_G, sl::kFeatureReflex, sl::kFeaturePCL };
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
    }

    void FrameGenerationHandler::initializeStreamline(EngineDevice& _device)
    {
        m_vkInfo.instance = _device.instance();
        m_vkInfo.physicalDevice = _device.physicalDevice();
        m_vkInfo.device = _device.device();

        QueueFamilyIndices indices = _device.findPhysicalQueueFamilies();
        m_vkInfo.graphicsQueueFamily = indices.m_graphicsFamily;
        m_vkInfo.graphicsQueueIndex = _device.hostGraphicsQueuesInFamily();
        m_vkInfo.computeQueueFamily = indices.m_graphicsFamily;
        m_vkInfo.computeQueueIndex = _device.hostGraphicsQueuesInFamily();
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

        slSetFeatureLoaded(sl::kFeatureReflex, true);
        slSetFeatureLoaded(sl::kFeaturePCL, true);
        slSetFeatureLoaded(sl::kFeatureDLSS, false);

        sl::ReflexOptions ro{};
        ro.mode = sl::ReflexMode::eLowLatency;
        sl::Result r = slReflexSetOptions(ro);
        if (r != sl::Result::eOk) 
           printf("[SL] slReflexSetOptions failed: %d\n", (int)r);

        m_DLSSGOptions.numFramesToGenerate = 3;
        setDLSSGOptions(true);
    }

    void FrameGenerationHandler::getFrameStats(FrameStats& _stats) const
    {
        _stats.m_totalPresentedFrameCount = m_lastState.numFramesActuallyPresented;
        _stats.m_isFrameGenerationEnabled = m_DLSSGOptions.mode != sl::DLSSGMode::eOff;
    }

    void FrameGenerationHandler::updateState()
    {
        if (!m_seenFirstPresent) return;
        if (SL_FAILED(res, slDLSSGGetState(m_viewport, m_lastState, nullptr)))
            printf("[SL] slDLSSGGetState failed: %d\n", (int)res);
    }

    void FrameGenerationHandler::reflexPresentStart(const sl::FrameToken& _frameToken)
    {
        slPCLSetMarker(sl::PCLMarker::ePresentStart, _frameToken);
    }
    void FrameGenerationHandler::reflexPresentEnd(const sl::FrameToken& _frameToken)
    {
        slPCLSetMarker(sl::PCLMarker::ePresentEnd, _frameToken);
    }

    void FrameGenerationHandler::reflexRenderSubmitStart(const sl::FrameToken & _frameToken)
    {
       slPCLSetMarker(sl::PCLMarker::eRenderSubmitStart, _frameToken);
    }
    void FrameGenerationHandler::reflexRenderSubmitEnd(const sl::FrameToken & _frameToken)
    {
       slPCLSetMarker(sl::PCLMarker::eRenderSubmitEnd, _frameToken);
    }
    
    void FrameGenerationHandler::reflexSimulationStart(const sl::FrameToken & _frameToken)
    {
       slPCLSetMarker(sl::PCLMarker::eSimulationStart, _frameToken);
    }
    void FrameGenerationHandler::reflexSimulationEnd(const sl::FrameToken & _frameToken)
    {
       slPCLSetMarker(sl::PCLMarker::eSimulationEnd, _frameToken);
    }

    // Helper: convert glm::mat4 (column-major) -> sl::float4x4 (row-major)
    static sl::float4x4 toSL(const glm::mat4& m)
    {
        sl::float4x4 row{};
        row[0] = sl::float4(m[0][0], m[1][0], m[2][0], m[3][0]);
        row[1] = sl::float4(m[0][1], m[1][1], m[2][1], m[3][1]);
        row[2] = sl::float4(m[0][2], m[1][2], m[2][2], m[3][2]);
        row[3] = sl::float4(m[0][3], m[1][3], m[2][3], m[3][3]);
        return row;
    }

    void FrameGenerationHandler::setCommonConstants(const glm::mat4& _viewMatrix, const glm::mat4& _projectionMatrix,
        const glm::mat4& _prevViewMatrix, const glm::mat4& _prevProjectionMatrix,
        VkExtent2D _renderSize, VkExtent2D _displaySize,
        float _nearZ, float _farZ, bool _depthInverted,
        glm::vec2 _motionVecScale)
    {
        // Build required transforms for v2 Constants
        const glm::mat4 invView = glm::inverse(_viewMatrix);
        const glm::mat4 invProj = glm::inverse(_projectionMatrix);
        const glm::mat4 viewToViewPrev = _prevViewMatrix * invView;
        const glm::mat4 clipToPrevClipM = _prevProjectionMatrix * _prevViewMatrix * invView * invProj;
        const glm::mat4 prevClipToClipM = glm::inverse(clipToPrevClipM);

        sl::Constants c{};
        c.cameraViewToClip = toSL(_projectionMatrix);
        c.clipToCameraView = toSL(invProj);
        c.clipToPrevClip = toSL(clipToPrevClipM);
        c.prevClipToClip = toSL(prevClipToClipM);
        c.jitterOffset = sl::float2(0, 0);
        c.mvecScale = sl::float2(_motionVecScale.x, _motionVecScale.y);
        c.cameraNear = _nearZ;
        c.cameraFar = _farZ;
        c.depthInverted = _depthInverted ? sl::Boolean::eTrue : sl::Boolean::eFalse;
        c.cameraMotionIncluded = sl::Boolean::eTrue; // MVs include camera motion
        c.motionVectors3D = sl::Boolean::eFalse;
        c.motionVectorsDilated = sl::Boolean::eFalse;
        c.motionVectorsJittered = sl::Boolean::eFalse;
        c.orthographicProjection = sl::Boolean::eFalse;


        const glm::vec3 camPos = glm::vec3(invView[3]);
        const glm::vec3 camRight = glm::normalize(glm::vec3(invView[0]));
        const glm::vec3 camUp = glm::normalize(glm::vec3(invView[1]));
        const glm::vec3 camFwd = glm::normalize(-glm::vec3(invView[2])); // -Z forward
        const float aspect = (_displaySize.height > 0) ? float(_displaySize.width) / float(_displaySize.height) : 1.0f;
        const float fY = _projectionMatrix[1][1];
        const float fov = 2.0f * atanf(1.0f / std::max(fY, 1e-6f)); // radians

        c.cameraPos = sl::float3(camPos.x, camPos.y, camPos.z);
        c.cameraRight = sl::float3(camRight.x, camRight.y, camRight.z);
        c.cameraUp = sl::float3(camUp.x, camUp.y, camUp.z);
        c.cameraFwd = sl::float3(camFwd.x, camFwd.y, camFwd.z);
        c.cameraFOV = fov;
        c.cameraAspectRatio = aspect;
        c.cameraPinholeOffset = sl::float2(0.0f, 0.0f); // No lens shift

        c.reset = (m_resetFrames > 0) ? sl::Boolean::eTrue : sl::Boolean::eFalse;
        //c.reset = sl::Boolean::eFalse;

        if (SL_FAILED(res, slGetNewFrameToken(m_frameToken)))
        {
            throw std::runtime_error("Failed to get new frame token for Streamline: " + std::to_string(static_cast<int>(res)));
        }

        if (SL_FAILED(res, slSetConstants(c, getFrameToken(), m_viewport)))
        {
            throw std::runtime_error("Failed to set common constants for Streamline: " + std::to_string(static_cast<int>(res)));
        }

        m_lastConstants = c;

        if (m_resetFrames > 0) m_resetFrames--;
    }

    void FrameGenerationHandler::tagResources(VkImage _depth, VkImageView _depthView, VkDeviceMemory _depthMem,
        VkImage _motionVec, VkImageView _motionVecView, VkDeviceMemory _motionVecMem,
        VkImage _hudlessColour, VkImageView _hudlessColourView, VkDeviceMemory _hudlessColourMem,
        VkExtent2D _extent, VkCommandBuffer _cmd)
    {
        sl::Resource rDepth{ sl::ResourceType::eTex2d, (void*)_depth, (void*)_depthMem, (void*)_depthView, (uint32_t)VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        sl::Resource rMotionVec{ sl::ResourceType::eTex2d, (void*)_motionVec, (void*)_motionVecMem, (void*)_motionVecView, (uint32_t)VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        sl::Resource rHudlessCol{ sl::ResourceType::eTex2d, (void*)_hudlessColour, (void*)_hudlessColourMem, (void*)_hudlessColourView, (uint32_t)VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

        rDepth.nativeFormat = (uint32_t)VK_FORMAT_D32_SFLOAT;
        rMotionVec.nativeFormat = (uint32_t)VK_FORMAT_R16G16_SFLOAT;
        rHudlessCol.nativeFormat = (uint32_t)VK_FORMAT_B8G8R8A8_UNORM;

        rDepth.width = rMotionVec.width = rHudlessCol.width = _extent.width;
        rDepth.height = rMotionVec.height = rHudlessCol.height = _extent.height;

        sl::Extent full{};
        full.width = _extent.width;
        full.height = _extent.height;

        sl::ResourceTag tags[] = {
            sl::ResourceTag{ &rDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &full },
            sl::ResourceTag{ &rMotionVec, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &full },
            sl::ResourceTag{ &rHudlessCol, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &full }
        };

        // if (SL_FAILED(res, slSetTagForFrame(getFrameToken(), m_viewport, tags, (uint32_t)std::size(tags), (void*)_cmd)))
        if (SL_FAILED(res, slSetTagForFrame(getFrameToken(), m_viewport, tags, (uint32_t)std::size(tags), reinterpret_cast<sl::CommandBuffer*>(_cmd))))
            printf("[SL] slSetTagForFrame failed: %d\n", (int)res);
    }

    void FrameGenerationHandler::setDLSSGOptions(const bool _enable)
    {
        if (_enable)
        {
            slSetFeatureLoaded(sl::kFeatureDLSS_G, true);
            m_DLSSGOptions.mode = sl::DLSSGMode::eOn;
        }
        else
        {
            slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
            m_DLSSGOptions.mode = sl::DLSSGMode::eOff;
        }

        if (SL_FAILED(res, slDLSSGSetOptions(m_viewport, m_DLSSGOptions)))
        {
            throw std::runtime_error("DLSS set options failed: " + std::to_string(static_cast<int>(res)));
        }
    }

    void FrameGenerationHandler::evaluateFeature(VkCommandBuffer _cmd)
    {
        if (!m_frameToken) return;

        // Cast Vulkan command buffer to SL type
        sl::CommandBuffer* cmd = reinterpret_cast<sl::CommandBuffer*>(_cmd);

        const sl::BaseStructure* inputs[] = { 
            static_cast<const sl::BaseStructure*>(&m_viewport),
            static_cast<const sl::BaseStructure*>(&m_lastConstants) 
        };
        const uint32_t numInputs = (uint32_t)std::size(inputs);

        if (SL_FAILED(res, slEvaluateFeature(sl::kFeatureDLSS_G, *m_frameToken, inputs, numInputs, cmd)))
        {
            //throw std::runtime_error("slEvaluateFeature failed with code: " + std::to_string(static_cast<int>(res)));
        }
    }
}