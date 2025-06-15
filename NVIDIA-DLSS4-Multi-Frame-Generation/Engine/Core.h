#pragma once
#include "Window.h"
#include "Pipeline.h"

#include <memory>

namespace Engine
{
    struct Core 
    {
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        static std::shared_ptr<Core> initialize();
        void run();
        void stop();

    private:
        bool terminateApplication;

        std::shared_ptr<EngineWindow> window;
        std::shared_ptr<Pipeline> pipeline;
    };
}