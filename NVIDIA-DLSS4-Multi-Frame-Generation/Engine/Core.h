#pragma once
#include "Renderer.h"
#include "RenderSystem.h"
#include "InputHandler.h"

#include <memory>
#include <chrono>

namespace Engine
{
    struct Core 
    {
        // Window dimensions
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

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
        EngineDevice m_device{ m_window };
        Renderer m_renderer{ m_window, m_device };

        void loadGameObjects();
        std::vector<GameObject> m_gameObjects;
    };
}