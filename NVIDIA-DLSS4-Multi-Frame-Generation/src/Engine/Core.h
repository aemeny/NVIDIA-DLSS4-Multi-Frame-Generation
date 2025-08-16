#pragma once
#include "Renderer.h"
#include "..\Systems\RenderSystem.h"
#include "InputHandler.h"
#include "Descriptors.h"
#include "FrameGenerationHandler.h"

#include <memory>
#include <chrono>

namespace Engine
{
    struct Core 
    {
        // Window dimensions
        static constexpr int WIDTH = 1200;
        static constexpr int HEIGHT = 900;

        Core(std::shared_ptr<EngineWindow> _window);
        ~Core();
        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        void run();
        void stop();

    private:
        bool m_terminateApplication;

        // Member objs
        std::shared_ptr<EngineWindow> m_window;
        FrameGenerationHandler m_frameGenerationHandler{};
        EngineDevice m_device{ m_window, m_frameGenerationHandler };
        Renderer m_renderer{ m_window, m_device };
        std::unique_ptr<DescriptorPool> m_globalPool{};
        std::vector<std::unique_ptr<DescriptorPool>> framePools;

        void loadGameObjects();
        GameObject::Map m_gameObjects;
    };
}