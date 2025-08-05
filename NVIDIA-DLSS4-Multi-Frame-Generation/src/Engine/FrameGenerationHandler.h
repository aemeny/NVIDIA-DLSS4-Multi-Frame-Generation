#pragma once
#include "EngineDevice.h"

namespace Engine
{
    struct FrameGenerationHandler
    {
        FrameGenerationHandler();
        ~FrameGenerationHandler();

        FrameGenerationHandler(const FrameGenerationHandler&) = delete;
        FrameGenerationHandler& operator=(const FrameGenerationHandler&) = delete;
    
        void initializeStreamline(EngineDevice& _device);
        void shutDownStreamline();
    };
}   