#pragma once
#include "EngineDevice.h"

#include <Streamline/sl_dlss_g.h>
#include <Streamline/sl_helpers_vk.h>
#include <Streamline/sl_pcl.h>
#include <glm/matrix.hpp>

namespace Engine
{
    struct FrameStats
    {
        uint64_t m_totalPresentedFrameCount = 0;
        bool m_isFrameGenerationEnabled = false;
    };

    struct FrameGenerationHandler
    {
        FrameGenerationHandler();
        ~FrameGenerationHandler();

        FrameGenerationHandler(const FrameGenerationHandler&) = delete;
        FrameGenerationHandler& operator=(const FrameGenerationHandler&) = delete;
    
        void generatePreferences();
        void initializeStreamline(EngineDevice& _device);
        void shutDownStreamline();

        const sl::FrameToken& getFrameToken() const { return *m_frameToken; }
        void getFrameStats(FrameStats& _stats) const;
        void updateState();

        void reflexPresentStart(const sl::FrameToken& _frameToken);
        void reflexPresentEnd(const sl::FrameToken& _frameToken);
        void reflexRenderSubmitStart(const sl::FrameToken& _frameToken);
        void reflexRenderSubmitEnd(const sl::FrameToken & _frameToken);
        void reflexSimulationStart(const sl::FrameToken & _frameToken);
        void reflexSimulationEnd(const sl::FrameToken & _frameToken);

        void setCommonConstants(
            const glm::mat4& _viewMatrix, const glm::mat4& _projectionMatrix,
            const glm::mat4& _prevViewMatrix, const glm::mat4& _prevProjectionMatrix,
            VkExtent2D _renderSize, VkExtent2D _displaySize, 
            float _nearZ, float _farZ, bool _depthInverted,
            glm::vec2 _motionVecScale
        );
        void tagResources(
            VkImage _depth, VkImageView _depthView, VkDeviceMemory _depthMem,
            VkImage _motionVec, VkImageView _motionVecView, VkDeviceMemory _motionVecMem,
            VkImage _hudlessColour, VkImageView _hudlessColourView, VkDeviceMemory _hudlessColourMem,
            VkExtent2D _extent, VkCommandBuffer _cmd
        );

        void setDLSSGOptions(const bool _enable);
         
        void triggerReset(uint32_t _frames = 2) { m_resetFrames = _frames; }
        void markPresented() { m_seenFirstPresent = true; };

        void evaluateFeature(VkCommandBuffer _cmd);

        sl::DLSSGOptions m_DLSSGOptions{};
        sl::DLSSGState m_lastState{};
        sl::FrameToken* m_frameToken = nullptr;

    private:
        sl::Preferences m_preferences{};
        sl::VulkanInfo m_vkInfo{};

        sl::Constants m_lastConstants{};
        sl::ViewportHandle m_viewport = sl::ViewportHandle(0);

        bool m_seenFirstPresent = false;
        uint32_t m_resetFrames = 2;

        uint64_t m_frameIndex = 0;
    };
}   