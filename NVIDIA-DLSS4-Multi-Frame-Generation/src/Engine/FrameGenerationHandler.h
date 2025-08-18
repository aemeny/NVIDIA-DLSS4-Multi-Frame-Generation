#pragma once
#include "EngineDevice.h"

#include <Streamline/sl_dlss_g.h>
#include <Streamline/sl_helpers_vk.h>

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
    
        void initializeStreamline(EngineDevice& _device);
        void shutDownStreamline();

        bool getFrameStats(FrameStats& _stats) const;

    private:
        sl::Preferences m_preferences{};
        sl::VulkanInfo m_vkInfo{};
        sl::DLSSGOptions m_DLSSGOptions{};
    };
}   