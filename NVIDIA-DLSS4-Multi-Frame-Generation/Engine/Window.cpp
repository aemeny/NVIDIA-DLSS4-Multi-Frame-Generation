#include "Window.h"

namespace Engine
{
    EngineWindow::EngineWindow(int _width, int _height, std::string _name) :
        width(_width), height(_height), windowName(_name)
    {
        initWindow();
    }

    EngineWindow::~EngineWindow()
    {
        if (window)
        {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }

    void EngineWindow::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context needed for Vulkan
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable window resizing

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }
}
