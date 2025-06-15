#include "Core.h"

//std
#include <iostream>
#include <cstdlib>
#include <stdexcept>

using namespace Engine; // Namespace for the custom engine for readable code
int main() {
  
    // Initialize the engine core
    std::shared_ptr<Core> engineCore = Core::initialize();

    try 
    {
        engineCore->run();
    } 
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}