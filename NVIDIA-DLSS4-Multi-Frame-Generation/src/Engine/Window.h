#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Engine
{
    struct EngineDevice;
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
        void setDevice(EngineDevice* _device) { m_device = _device; }

        void createWindowSurface(VkInstance _instance, VkSurfaceKHR* _surface);

        void setFullscreen(bool _enable, bool _borderless = true, GLFWmonitor* _monitor = nullptr, int _refreshRate = GLFW_DONT_CARE);
        void toggleFullscreen() { setFullscreen(!m_isFullscreen, m_borderless); }
        bool isFullscreen() const { return m_isFullscreen; }

    private:
        static void frameBufferResizeCallback(GLFWwindow* _window, int _width, int _height);
        void initWindow();

        bool m_isFullscreen = false;
        bool m_borderless = true;
        int m_width;
        int m_height;
        int m_windowCordX;
        int m_windowCordY;
        bool m_framebufferResized = false; // Flag to check if the window has been resized
        EngineDevice* m_device;

        std::string m_windowName;

        GLFWwindow* m_window;
    };
}