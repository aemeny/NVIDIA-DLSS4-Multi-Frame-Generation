#include "Pipeline.h"
#include "ModelHandler.h"

#include <fstream>
#include <iostream>
#include <cassert>

namespace Engine
{
    Pipeline::Pipeline(EngineDevice& _device, const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo)
        : m_device(_device)
    {
        createGraphicsPipeline(_vertFilePath, _fragFilePath, _configInfo);
    }

    Pipeline::~Pipeline()
    {
        vkDestroyShaderModule(m_device.device(), m_vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device.device(), m_fragShaderModule, nullptr);
        vkDestroyPipeline(m_device.device(), m_graphicsPipeline, nullptr);
    }

    void Pipeline::createGraphicsPipeline(const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo)
    {
        assert(_configInfo.m_pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline: No pipelineLayout provided");
        assert(_configInfo.m_renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline: No renderPass provided");

        auto vertCode = readFile(_vertFilePath);
        auto fragCode = readFile(_fragFilePath);

        createShaderModule(vertCode, &m_vertShaderModule);
        createShaderModule(fragCode, &m_fragShaderModule);

        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = m_vertShaderModule;
        shaderStages[0].pName = "main"; // Entry point for vertex shader
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = m_fragShaderModule;
        shaderStages[1].pName = "main"; // Entry point for vertex shader
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        auto& attributeDescriptions = _configInfo.m_attributeDescriptions;
        auto& bindingDescription = _configInfo.m_bindingDescriptions;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2; // Two shader stages (vertex and fragment)
        pipelineInfo.pStages = shaderStages; // Pointer to shader stages
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &_configInfo.m_inputAssemblyInfo;
        pipelineInfo.pViewportState = &_configInfo.m_viewportInfo;
        pipelineInfo.pRasterizationState = &_configInfo.m_rasterizationInfo;
        pipelineInfo.pMultisampleState = &_configInfo.m_multiSampleInfo;
        pipelineInfo.pColorBlendState = &_configInfo.m_colorBlendInfo;
        pipelineInfo.pDepthStencilState = &_configInfo.m_depthStencilInfo;
        pipelineInfo.pDynamicState = &_configInfo.m_dynamicStateInfo;

        pipelineInfo.layout = _configInfo.m_pipelineLayout;
        pipelineInfo.renderPass = _configInfo.m_renderPass;
        pipelineInfo.subpass = _configInfo.m_subpass;

        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");
    }

    void Pipeline::createShaderModule(const std::vector<char>& _code, VkShaderModule* _shaderModule)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = _code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

        if (vkCreateShaderModule(m_device.device(), &createInfo, nullptr, _shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module!");
    }

    void Pipeline::defaultPipelineConfigInfo(PipelineConfigInfo& _configInfo)
    {
        // Input Assembly State (First step in pipeline)
       _configInfo.m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
       _configInfo.m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Default to triangle list
       _configInfo.m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // No primitive restart


       // Viewport
       _configInfo.m_viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
       _configInfo.m_viewportInfo.viewportCount = 1;
       _configInfo.m_viewportInfo.pViewports = nullptr;
       _configInfo.m_viewportInfo.scissorCount = 1;
       _configInfo.m_viewportInfo.pScissors = nullptr;


       // Rasterization State (Converts vertices to fragments)
       _configInfo.m_rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
       _configInfo.m_rasterizationInfo.depthClampEnable = VK_FALSE; // No depth clamping
       _configInfo.m_rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // No discarding of rasterized fragments
       _configInfo.m_rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
       _configInfo.m_rasterizationInfo.lineWidth = 1.0f;
       _configInfo.m_rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // Backface culling
       _configInfo.m_rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Counter-clockwise as front
       _configInfo.m_rasterizationInfo.depthBiasEnable = VK_FALSE; // No depth bias
       _configInfo.m_rasterizationInfo.depthBiasConstantFactor = 0.0f;
       _configInfo.m_rasterizationInfo.depthBiasClamp = 0.0f;
       _configInfo.m_rasterizationInfo.depthBiasSlopeFactor = 0.0f;
       
       
       // MultiSample State (Anti-aliasing)
       _configInfo.m_multiSampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
       _configInfo.m_multiSampleInfo.sampleShadingEnable = VK_FALSE;
       _configInfo.m_multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
       _configInfo.m_multiSampleInfo.minSampleShading = 1.0f;
       _configInfo.m_multiSampleInfo.pSampleMask = nullptr;
       _configInfo.m_multiSampleInfo.alphaToCoverageEnable = VK_FALSE;
       _configInfo.m_multiSampleInfo.alphaToOneEnable = VK_FALSE;
       
       
       // Color Blend State (Combines fragment colors)
       _configInfo.m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
       _configInfo.m_colorBlendAttachment.blendEnable = VK_FALSE;
       _configInfo.m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
       _configInfo.m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
       _configInfo.m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
       _configInfo.m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
       _configInfo.m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
       _configInfo.m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
       
       _configInfo.m_colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
       _configInfo.m_colorBlendInfo.logicOpEnable = VK_FALSE;
       _configInfo.m_colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
       _configInfo.m_colorBlendInfo.attachmentCount = 1;
       _configInfo.m_colorBlendInfo.pAttachments = &_configInfo.m_colorBlendAttachment;
       _configInfo.m_colorBlendInfo.blendConstants[0] = 0.0f;
       _configInfo.m_colorBlendInfo.blendConstants[1] = 0.0f;
       _configInfo.m_colorBlendInfo.blendConstants[2] = 0.0f;
       _configInfo.m_colorBlendInfo.blendConstants[3] = 0.0f;
       
       
       // Depth Stencil State (Depth testing)
       _configInfo.m_depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
       _configInfo.m_depthStencilInfo.depthTestEnable = VK_TRUE;
       _configInfo.m_depthStencilInfo.depthWriteEnable = VK_TRUE;
       _configInfo.m_depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
       _configInfo.m_depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
       _configInfo.m_depthStencilInfo.minDepthBounds = 0.0f;
       _configInfo.m_depthStencilInfo.maxDepthBounds = 1.0f;
       _configInfo.m_depthStencilInfo.stencilTestEnable = VK_FALSE;
       _configInfo.m_depthStencilInfo.front = {};
       _configInfo.m_depthStencilInfo.back = {};

       // Dynamic State (States that can change at runtime)
       _configInfo.m_dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
       _configInfo.m_dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
       _configInfo.m_dynamicStateInfo.pDynamicStates = _configInfo.m_dynamicStateEnables.data();
       _configInfo.m_dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_configInfo.m_dynamicStateEnables.size());
       _configInfo.m_dynamicStateInfo.flags = 0;


       _configInfo.m_bindingDescriptions = Model::Vertex::getBindingDescriptions();
       _configInfo.m_attributeDescriptions = Model::Vertex::getAttributeDescriptions();
    }

    void Pipeline::enableAlphaBlending(PipelineConfigInfo& _configInfo)
    {
        _configInfo.m_colorBlendAttachment.blendEnable = VK_TRUE;
        _configInfo.m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        _configInfo.m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        _configInfo.m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        _configInfo.m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        _configInfo.m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        _configInfo.m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        _configInfo.m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void Pipeline::bind(VkCommandBuffer _commandBuffer)
    {
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    }

    std::vector<char> Pipeline::readFile(const std::string& _filePath)
    {
        std::ifstream file(_filePath, std::ios::ate | std::ios::binary); // ate: start reading from the end (getting size easier), binary: read as binary file

        if (!file.is_open())
            throw std::runtime_error("Failed to open file: " + _filePath);

        size_t fileSize = static_cast<size_t>(file.tellg()); // Get the current position in the file (End of file)
        std::vector<char> buffer(fileSize); // Create a buffer of the file size

        file.seekg(0); // Move the file pointer back to the beginning of the file
        file.read(buffer.data(), fileSize); // Read the file into the buffer

        file.close();
        return buffer;
    }
}