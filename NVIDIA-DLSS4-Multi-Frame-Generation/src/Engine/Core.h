#pragma once
#include "Renderer.h"
#include "..\Systems\RenderSystem.h"
#include "InputHandler.h"
#include "Descriptors.h"
#include "FrameGenerationHandler.h"
#include "SceneTester.h"

#include <memory>
#include <chrono>

namespace Engine
{
    struct Core 
    {
        // Window dimensions
        static constexpr int WIDTH = 1920;
        static constexpr int HEIGHT = 1080;

        Core(std::shared_ptr<EngineWindow> _window);
        ~Core();
        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        void run();
        void stop();

    private:
        bool m_terminateApplication;

        // Member objs
        FrameGenerationHandler m_frameGenerationHandler{};
        std::shared_ptr<EngineWindow> m_window;
        SlVkProxies m_slProxies;
        EngineDevice m_device{ m_window, m_frameGenerationHandler, m_slProxies};
        Renderer m_renderer{ m_window, m_device, m_slProxies };
        std::unique_ptr<DescriptorPool> m_globalPool{};
        std::vector<std::unique_ptr<DescriptorPool>> framePools;

        SceneTester::CameraPanController m_panCameraController{};
        SceneTester::SceneLoader m_loader{ m_device };

        void loadGameObjects();
        GameObject::Map m_gameObjects;
        glm::mat4 m_prevViewMatrix{1.0f};
        glm::mat4 m_prevProjectionMatrix{1.0f};
    };
}