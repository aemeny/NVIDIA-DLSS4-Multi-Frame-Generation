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

        bool shouldClose() const { return glfwWindowShouldClose(window); }

    private:
        void initWindow();

        const int width;
        const int height;

        std::string windowName;

        GLFWwindow* window;
    };
}