#pragma once
#include "EngineDevice.h"

#include <string>
#include <vector>

namespace Engine
{
    struct PipelineConfigInfo 
    {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

        VkPipelineViewportStateCreateInfo m_viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo m_rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo m_multisampleInfo;
        VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo m_colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;
        std::vector<VkDynamicState> m_dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo m_dynamicStateInfo;
        VkPipelineLayout m_pipelineLayout = nullptr;
        VkRenderPass m_renderPass = nullptr;
        uint32_t m_subpass = 0;
    };

    struct Pipeline
    {
        Pipeline(EngineDevice& _device, const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo);
        Pipeline() = default;
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        static void defaultPipelineConfigInfo(PipelineConfigInfo& _configInfo);

        void bind(VkCommandBuffer _commandBuffer);

    private:
        static std::vector<char> readFile(const std::string& _filePath);

        void createGraphicsPipeline(const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo);

        void createShaderModule(const std::vector<char>& _code, VkShaderModule* _shaderModule);

        EngineDevice& m_device;
        VkPipeline m_graphicsPipeline;
        VkShaderModule m_vertShaderModule;
        VkShaderModule m_fragShaderModule;
    };
}