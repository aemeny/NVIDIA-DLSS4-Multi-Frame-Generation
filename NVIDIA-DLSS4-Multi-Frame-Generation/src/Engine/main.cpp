// Engine Core
#include "Core.h" 

#include <iostream>
#include <cstdlib>

// Namespace from the custom engine for readability
using namespace Engine; 
int main() 
{
    // Initialize the engine core
    Core engineCore(std::make_shared<EngineWindow>(Core::WIDTH, Core::HEIGHT, "Vulkan Engine"));

    try 
    {
        engineCore.run();
    } 
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}