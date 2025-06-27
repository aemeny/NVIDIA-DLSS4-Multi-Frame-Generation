#include "Window.h"

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
    }
}
