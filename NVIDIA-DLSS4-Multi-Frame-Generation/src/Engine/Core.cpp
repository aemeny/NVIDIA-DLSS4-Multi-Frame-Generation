#include "Core.h"
#include "Buffer.h"
#include "..\Systems\PointLightSystem.h"
#include "..\Systems\TextureRenderSystem.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <iostream>

namespace Engine
{
    Core::Core(std::shared_ptr<EngineWindow> _window)
        : m_window(_window) 
    {
        m_globalPool = DescriptorPool::Builder(m_device)
            .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        // build frame descriptor pools
        framePools.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        auto framePoolBuilder = DescriptorPool::Builder(m_device)
            .setMaxSets(1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        for (int i = 0; i < framePools.size(); i++) 
        {
            framePools[i] = framePoolBuilder.build();
        }

        m_renderer.setFrameGen(&m_frameGenerationHandler);

        loadGameObjects();
    }

    Core::~Core(){}

    void Core::run()
    {
        std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++)
        {
            uboBuffers[i] = std::make_unique<Buffer>(
                m_device,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            uboBuffers[i]->map();
        }

        auto globalSetLayout = DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++)
        {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            DescriptorWriter(*globalSetLayout, *m_globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        RenderSystem renderSystem(m_device, m_renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
        PointLightSystem pointLightSystem(m_device, m_renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
        TextureRenderSystem textureRenderSystem(m_device, m_renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());

        Camera camera{};
        camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.0f));
        GameObject viewerObject = GameObject::createGameObject(); // Camera object used to store the state
        viewerObject.m_transform.m_translation = glm::vec3(0.0f, 0.0f, -2.5f);

        InputHandler inputHandler{};

        // Delta time tracking
        auto currentTime = std::chrono::high_resolution_clock::now();
        double fpsAccumTime = 0.0;
        uint64_t accumFrames = 0;

        m_terminateApplication = false;
        while (!m_window->shouldClose() && !m_terminateApplication)
        {
            // Poll events
            glfwPollEvents();

            // Record delta time
            auto newTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            // Update previous matrices
            m_prevViewMatrix = camera.getViewMatrix();
            m_prevProjectionMatrix = camera.getProjectionMatrix();
            for (auto& [id, obj] : m_gameObjects) 
            {
                obj.m_transform.m_prevModelMatrix = obj.m_transform.mat4();
            }

            // Gather Input and update camera
            if (m_loader.m_sceneType == SceneTester::SceneType::CameraPan)
            {
                m_panCameraController.update(deltaTime, viewerObject);
            }
            else
            {
                inputHandler.moveInPlaneXZ(m_window->getGLFWWindow(), deltaTime, viewerObject);
            }
            camera.setViewYXZ(viewerObject.m_transform.m_translation, viewerObject.m_transform.m_rotation);

            // Update Camera aspect ratio and projection
            float aspectRatio = m_renderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);

            if (m_loader.m_sceneType == SceneTester::SceneType::MovingScene) 
            {
                m_loader.updateMovingScene(deltaTime, m_gameObjects);
            }

            // Render
            if (VkCommandBuffer commandBuffer = m_renderer.beginFrame())
            {
                int frameIndex = m_renderer.getCurrentFrameIndex();
                framePools[frameIndex]->resetPool();
                FrameInfo frameInfo{
                    frameIndex,
                    deltaTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    *framePools[frameIndex],
                    m_gameObjects
                };

                // Update
                GlobalUbo ubo{};
                ubo.m_projection = camera.getProjectionMatrix();
                ubo.m_view = camera.getViewMatrix();
                ubo.m_inverseView = camera.getInverseViewMatrix();
                ubo.m_prevProjection = m_prevProjectionMatrix;
                ubo.m_prevView = m_prevViewMatrix; 
                ubo.m_renderSize = { m_renderer.getSwapChainExtent().width, m_renderer.getSwapChainExtent().height };

                if (m_loader.m_sceneType == SceneTester::SceneType::CameraPan)
                {
                    pointLightSystem.update(frameInfo, ubo, false);
                }
                else
                {
                    pointLightSystem.update(frameInfo, ubo, true);
                }

                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Set common constants for Streamline
                m_renderer.pushSLCommonConstants(
                    camera.getViewMatrix(), camera.getProjectionMatrix(),
                    m_prevViewMatrix, m_prevProjectionMatrix,
                    0.1f, 100.0f,
                    false, // DepthInverted
                    glm::vec2(1.0f / m_renderer.getSwapChainExtent().width, 1.0f / m_renderer.getSwapChainExtent().height)
                );

                // Render
                m_renderer.beginSwapChainRenderPass(commandBuffer);

                // Rendering solid objects first
                textureRenderSystem.renderGameObjects(frameInfo);
                renderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);

                m_renderer.endSwapChainRenderPass(commandBuffer);
                m_renderer.endFrame();
            }

            fpsAccumTime += deltaTime;
            accumFrames += 1;

            if (fpsAccumTime >= 1.0)
            {
                FrameStats frameStats{};
                m_frameGenerationHandler.getFrameStats(frameStats);

                // Update title
                char title[256];
                if (frameStats.m_isFrameGenerationEnabled)
                {                    
                    uint64_t genframes = accumFrames * frameStats.m_totalPresentedFrameCount;
                    uint64_t totalFrames = accumFrames + genframes;
                    double percentIncrease = ((totalFrames - accumFrames) / accumFrames) * 100;

                    std::snprintf(title, sizeof(title),
                        "Vulkan Engine | Render: %d FPS | Output: %d FPS | FG: +%d FPS (+%.0f%%)",
                        accumFrames, totalFrames, genframes, percentIncrease);
                }
                else
                {
                    std::snprintf(title, sizeof(title),
                        "Vulkan Engine | Render: %d FPS (FG off)", 
                        accumFrames);
                }

                glfwSetWindowTitle(m_window->getGLFWWindow(), title);

                fpsAccumTime = 0.0;
                accumFrames = 0.0;
            }
        }
        vkDeviceWaitIdle(m_device.device()); // Wait for the device to finish all operations before exiting
        m_frameGenerationHandler.shutDownStreamline(); // Clean up Streamline resources before Vulkan shutdown
    }

    void Core::stop()
    {
        m_terminateApplication = true;
    }

    void Core::loadGameObjects()
    {
        SceneTester::SceneType type = SceneTester::SceneType::CameraPan;

        switch (type)
        {
        case SceneTester::SceneType::StaticGrid: // Static, GPU-heavy grid.
            m_loader.m_sceneType = type;
            m_loader.loadStaticGrid(m_gameObjects,
                100, 100, // Grid dimensions, X, Z
                0.25f, // Spacing
                1.0f, // Uniform scale
                0.0f, // Y position
                true // Add lights
            );
            break;

        case SceneTester::SceneType::CameraPan: // Camera orbiting, GPU-heavy grid.
            m_loader.m_sceneType = type;
            m_loader.loadStaticGrid(m_gameObjects,
                100, 100, // Grid dimensions, X, Z
                0.25f, // Spacing
                1.0f, // Uniform scale
                0.0f, // Y position
                true // Add lights
            );
            break;

        case SceneTester::SceneType::MovingScene: // Moving scene, GPU + CPU heavy.
            m_loader.m_sceneType = type;
            m_loader.loadMovingScene(
                m_gameObjects,
                100, 100, // Grid dimensions, X, Z
                0.20f, // Spacing
                1.0f, // Uniform scale
                0.0f // Y position
            );
            break;

        case SceneTester::SceneType::TrasnsparencyTest: // Transparency overdraw test.
            m_loader.m_sceneType = type;
            m_loader.loadTransparencyTest(
                m_gameObjects,
                500, // Number of quads
                0.0f, // Y position
                3.0f // Scale
            );
            break;
        }
    }
}