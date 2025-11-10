#include "Window.h"

#include "EngineDevice.h"

#include <stdexcept>

namespace Engine
{
    EngineWindow::EngineWindow(int _width, int _height, std::string _name) :
        m_width(_width), m_height(_height), m_windowName(_name)
    {
        initWindow();
    }

    EngineWindow::~EngineWindow()
    {
        if (m_window)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }

    void EngineWindow::createWindowSurface(VkInstance _instance, VkSurfaceKHR* _surface)
    {
        if (glfwCreateWindowSurface(_instance, m_window, nullptr, _surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface!");
    }

    void EngineWindow::frameBufferResizeCallback(GLFWwindow* _window, int _width, int _height)
    {
        auto window = reinterpret_cast<EngineWindow*>(glfwGetWindowUserPointer(_window));
        window->m_framebufferResized = true;
        window->m_width = _width;
        window->m_height = _height;
    }

    void EngineWindow::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context needed for Vulkan
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Window resizing

        m_window = glfwCreateWindow(m_width, m_height, m_windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, frameBufferResizeCallback); // When resized call this function

        glfwSetKeyCallback(m_window,
            [](GLFWwindow* _window, int _key, int, int _action, int)
            {
                if (_action == GLFW_PRESS && _key == GLFW_KEY_F11)
                {
                    auto* self = static_cast<EngineWindow*>(glfwGetWindowUserPointer(_window));
                    self->toggleFullscreen();
                }
            });
    }

    void EngineWindow::setFullscreen(bool _enable, bool _borderless, GLFWmonitor* _monitor, int _refreshRate)
    {
        if (m_isFullscreen == _enable) return;
        if (!_monitor) _monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(_monitor);

        if (_enable) 
        {
            glfwGetWindowPos(m_window, &m_windowCordX, &m_windowCordY);
            glfwGetWindowSize(m_window, &m_width, &m_height);
            m_borderless = _borderless;

            if (_borderless) 
            {
                // Borderless full screen
                glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
                glfwSetWindowAttrib(m_window, GLFW_AUTO_ICONIFY, GLFW_FALSE);
                glfwSetWindowMonitor(m_window, nullptr, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
                
                int mx, my; 
                glfwGetMonitorPos(_monitor, &mx, &my);
                glfwSetWindowPos(m_window, mx, my);

            }
            else 
            {
                // Exclusive full screen
                glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
                glfwSetWindowMonitor(m_window, _monitor, 0, 0, mode->width, mode->height,
                    (_refreshRate == GLFW_DONT_CARE) ? mode->refreshRate : _refreshRate);

            }
            m_isFullscreen = true;

        }
        else 
        {
            // Restore windowed mode
            glfwSetWindowMonitor(m_window, nullptr, m_windowCordX, m_windowCordY, m_width, m_height, GLFW_DONT_CARE);
            glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
            m_isFullscreen = false;

        }
        m_framebufferResized = true;
    }
}
