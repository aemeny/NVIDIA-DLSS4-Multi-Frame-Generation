#include "Core.h"
#include "Buffer.h"

#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <iostream>

namespace Engine
{
    struct GlobalUbo
    {
        glm::mat4 m_projectionView{ 1.0f };
        glm::vec3 m_lightDirection = glm::normalize(glm::vec3(1.0f, -3.0f, -1.0f));
    };

    Core::Core(std::shared_ptr<EngineWindow> _window)
        : m_window(_window) 
    {
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
        RenderSystem renderSystem(m_device, m_renderer.getSwapChainRenderPass());

        Camera camera{};
        camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.0f));
        GameObject viewerObject = GameObject::createGameObject(); // Camera object used to store the state

        InputHandler inputHandler{};


        auto currentTime = std::chrono::high_resolution_clock::now();
        m_terminateApplication = false;
        while (!m_window->shouldClose() && !m_terminateApplication)
        {
            // Poll events
            glfwPollEvents();

            // Record delta time
            auto newTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            // Gather Input and update camera
            inputHandler.moveInPlaneXZ(m_window->getGLFWWindow(), deltaTime, viewerObject);
            camera.setViewYXZ(viewerObject.m_transform.m_translation, viewerObject.m_transform.m_rotation);

            // Update Camera aspect ratio and projection
            float aspectRatio = m_renderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.0f), aspectRatio, 0.1f, 100.0f);
            
            // Render
            if (VkCommandBuffer commandBuffer = m_renderer.beginFrame())
            {
                int frameIndex = m_renderer.getCurrentFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    deltaTime,
                    commandBuffer,
                    camera
                };

                // Update
                GlobalUbo ubo{};
                ubo.m_projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();
                
                // Render
                m_renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(frameInfo, m_gameObjects);
                m_renderer.endSwapChainRenderPass(commandBuffer);
                m_renderer.endFrame();
            }
        }

        vkDeviceWaitIdle(m_device.device()); // Wait for the device to finish all operations before exiting
    }

    void Core::stop()
    {
        m_terminateApplication = true;
    }

    void Core::loadGameObjects()
    {
        std::shared_ptr<Model> model = Model::createModelFromFile(m_device, "Samples/Models/smooth_vase.obj");
        GameObject obj1 = GameObject::createGameObject();
        obj1.m_model = model;
        obj1.m_transform.m_translation = glm::vec3(-0.2f, 0.2f, 0.75f);
        obj1.m_transform.m_scale = glm::vec3(1.0f, 1.0f, 1.0f);

        m_gameObjects.push_back(std::move(obj1));

        model = Model::createModelFromFile(m_device, "Samples/Models/flat_vase.obj");
        GameObject obj2 = GameObject::createGameObject();
        obj2.m_model = model;
        obj2.m_transform.m_translation = glm::vec3(0.2f, 0.2f, 0.75f);
        obj2.m_transform.m_scale = glm::vec3(1.0f, 1.0f, 1.0f);

        m_gameObjects.push_back(std::move(obj2));
    }
}