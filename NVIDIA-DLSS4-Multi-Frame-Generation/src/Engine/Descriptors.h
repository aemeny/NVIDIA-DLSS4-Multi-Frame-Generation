#pragma once
#include "EngineDevice.h"

#include <unordered_map>
#include <stdint.h>

namespace Engine
{
    struct DescriptorSetLayout
    {
        struct Builder
        {
            Builder(EngineDevice& _device) : m_device{ _device } {}

            Builder& addBinding(uint32_t _binding, VkDescriptorType _descriptorType, VkShaderStageFlags _stageFlags, uint32_t _count = 1);
            std::unique_ptr<DescriptorSetLayout> build() const;

        private:
            EngineDevice& m_device;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
        };

        DescriptorSetLayout(EngineDevice& _device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _bindings);
        ~DescriptorSetLayout();
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

    private:
        EngineDevice& m_device;
        VkDescriptorSetLayout m_descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

        friend struct DescriptorWriter;
    };

    struct DescriptorPool 
    {
        struct Builder 
        {
            Builder(EngineDevice& _device) : m_device{ _device } {}

            Builder& addPoolSize(VkDescriptorType _descriptorType, uint32_t _count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags _flags);
            Builder& setMaxSets(uint32_t _count);
            std::unique_ptr<DescriptorPool> build() const;

        private:
            EngineDevice& m_device;
            std::vector<VkDescriptorPoolSize> m_poolSizes{};
            uint32_t m_maxSets = 1000;
            VkDescriptorPoolCreateFlags m_poolFlags = 0;
        };

        DescriptorPool(EngineDevice& m_device, uint32_t _maxSets, VkDescriptorPoolCreateFlags _poolFlags, const std::vector<VkDescriptorPoolSize>& _poolSizes);
        ~DescriptorPool();
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        bool allocateDescriptorSet(const VkDescriptorSetLayout _descriptorSetLayout, VkDescriptorSet& _descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& _descriptors) const;

        void resetPool();

    private:
        EngineDevice& m_device;
        VkDescriptorPool m_descriptorPool;

        friend struct DescriptorWriter;
    };

    struct DescriptorWriter 
    {
        DescriptorWriter(DescriptorSetLayout& _setLayout, DescriptorPool& _pool);

        DescriptorWriter& writeBuffer(uint32_t _binding, VkDescriptorBufferInfo* _bufferInfo);
        DescriptorWriter& writeImage(uint32_t _binding, VkDescriptorImageInfo* _imageInfo);

        bool build(VkDescriptorSet& _set);
        void overwrite(VkDescriptorSet& _set);

    private:
        DescriptorSetLayout& m_setLayout;
        DescriptorPool& m_pool;
        std::vector<VkWriteDescriptorSet> m_writes;
    };
}