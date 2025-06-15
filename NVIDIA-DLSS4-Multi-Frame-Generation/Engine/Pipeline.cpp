#include "Pipeline.h"
#include <fstream>
#include <iostream>

namespace Engine
{
    Pipeline::Pipeline(const std::string& vertFilePath, const std::string& fragFilePath)
    {
        createGraphicsPipeline(vertFilePath, fragFilePath);
    }

    void Pipeline::createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath)
    {
        auto vertCode = readFile(vertFilePath);
        auto fragCode = readFile(fragFilePath);

        // TEMPORARY: Print the sizes of the shader code to test
        std::cout << "Vertex shader code size: " << vertCode.size() << " bytes\n";
        std::cout << "Fragment shader code size: " << fragCode.size() << " bytes\n";
    }

    std::vector<char> Pipeline::readFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary); // ate: start reading from the end (getting size easier), binary: read as binary file

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg()); // Get the current position in the file (End of file)
        std::vector<char> buffer(fileSize); // Create a buffer of the file size

        file.seekg(0); // Move the file pointer back to the beginning of the file
        file.read(buffer.data(), fileSize); // Read the file into the buffer

        file.close();
        return buffer;
    }
}