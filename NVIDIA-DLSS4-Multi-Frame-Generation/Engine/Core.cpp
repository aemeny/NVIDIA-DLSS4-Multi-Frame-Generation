#include "Core.h"

namespace Engine
{
    std::shared_ptr<Core> Core::initialize()
    {
        std::shared_ptr<Core> rtn = std::make_shared<Core>();

        rtn->window = std::make_shared<EngineWindow>(WIDTH, HEIGHT, "Vulkan Engine");
        rtn->pipeline = std::make_shared<Pipeline>("Shaders/Unlit/Vertex.vert.spv", "Shaders/Unlit/Fragment.frag.spv");

        return rtn;
    }

    void Core::run()
    {
        terminateApplication = false;
        while (!window->shouldClose() && !terminateApplication)
        {
            glfwPollEvents();
        }
    }

    void Core::stop()
    {
        terminateApplication = true;
    }
}