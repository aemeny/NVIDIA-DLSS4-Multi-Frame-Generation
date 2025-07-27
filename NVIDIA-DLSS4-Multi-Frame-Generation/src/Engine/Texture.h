#pragma once
#include "EngineDevice.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <string>

namespace Engine
{
    struct Texture
    {
        Texture(EngineDevice& _device, const std::string& _textureFilePath);
        Texture(
            EngineDevice& _device,
            VkFormat _format,
            VkExtent3D _extent,
            VkImageUsageFlags _usage,
            VkSampleCountFlagBits _sampleCount);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        VkImageView imageView() const { return m_textureImageView; }
        VkSampler sampler() const { return m_textureSampler; }
        VkImage getImage() const { return m_textureImage; }
        VkImageView getImageView() const { return m_textureImageView; }
        VkDescriptorImageInfo getImageInfo() const { return m_descriptor; }
        VkImageLayout getImageLayout() const { return m_textureLayout; }
        VkExtent3D getExtent() const { return m_extent; }
        VkFormat getFormat() const { return m_format; }

        void updateDescriptor();
        void transitionLayout(VkCommandBuffer _commandBuffer, VkImageLayout _oldLayout, VkImageLayout _newLayout);

        static std::unique_ptr<Texture> createTextureFromFile(EngineDevice& _device, const std::string& _filePath);

    private:
        void createTextureImage(const std::string& _filePath);
        void createTextureImageView(VkImageViewType _viewType);
        void createTextureSampler();

        VkDescriptorImageInfo m_descriptor{};

        EngineDevice& m_device;
        VkImage m_textureImage = nullptr;
        VkDeviceMemory m_textureImageMemory = nullptr;
        VkImageView m_textureImageView = nullptr;
        VkSampler m_textureSampler = nullptr;
        VkFormat m_format;
        VkImageLayout m_textureLayout;
        uint32_t m_mipLevels{1};
        uint32_t m_layerCount{1};
        VkExtent3D m_extent{};
    };
}