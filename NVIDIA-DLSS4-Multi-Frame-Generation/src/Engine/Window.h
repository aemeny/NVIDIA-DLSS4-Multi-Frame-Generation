#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Engine
{
    struct EngineWindow
    {
        EngineWindow(int _width, int _height, std::string _name);
        ~EngineWindow();

        // Delete copy constructor and assignment operator to prevent copying
        EngineWindow(const EngineWindow&) = delete;
        EngineWindow& operator=(const EngineWindow&) = delete;

        bool shouldClose() const { return glfwWindowShouldClose(m_window); }
        VkExtent2D getExtent() { return { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) }; }
        bool hasWindowResized() const { return m_framebufferResized; }
        void resetWindowResizedFlag() { m_framebufferResized = false; }

        GLFWwindow* getGLFWWindow() const { return m_window; }

        void createWindowSurface(VkInstance _instance, VkSurfaceKHR* _surface);

    private:
        static void frameBufferResizeCallback(GLFWwindow* _window, int _width, int _height);
        void initWindow();

        int m_width;
        int m_height;
        bool m_framebufferResized = false; // Flag to check if the window has been resized

        std::string m_windowName;

        GLFWwindow* m_window;
    };
}