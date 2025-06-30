#include "Descriptors.h"

#include <cassert>
#include <stdexcept>

namespace Engine 
{

    // *************** Descriptor Set Layout Builder *********************

    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
        uint32_t _binding,
        VkDescriptorType _descriptorType,
        VkShaderStageFlags _stageFlags,
        uint32_t _count) 
    {
        assert(m_bindings.count(_binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = _binding;
        layoutBinding.descriptorType = _descriptorType;
        layoutBinding.descriptorCount = _count;
        layoutBinding.stageFlags = _stageFlags;
        m_bindings[_binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const 
    {
        return std::make_unique<DescriptorSetLayout>(m_device, m_bindings);
    }


    // *************** Descriptor Set Layout *********************

    DescriptorSetLayout::DescriptorSetLayout(EngineDevice& _device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _bindings)
        : m_device(_device), m_bindings(_bindings) 
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : _bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
            m_device.device(),
            &descriptorSetLayoutInfo,
            nullptr,
            &m_descriptorSetLayout) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() 
    {
        vkDestroyDescriptorSetLayout(m_device.device(), m_descriptorSetLayout, nullptr);
    }


    // *************** Descriptor Pool Builder *********************

    DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(VkDescriptorType _descriptorType, uint32_t _count) 
    {
        m_poolSizes.push_back({ _descriptorType, _count });
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags _flags) 
    {
        m_poolFlags = _flags;
        return *this;
    }
    DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t _count) 
    {
        m_maxSets = _count;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const 
    {
        return std::make_unique<DescriptorPool>(m_device, m_maxSets, m_poolFlags, m_poolSizes);
    }

    // *************** Descriptor Pool *********************

    DescriptorPool::DescriptorPool(
        EngineDevice& _device,
        uint32_t _maxSets,
        VkDescriptorPoolCreateFlags _poolFlags,
        const std::vector<VkDescriptorPoolSize>& _poolSizes)
        : m_device(_device) 
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(_poolSizes.size());
        descriptorPoolInfo.pPoolSizes = _poolSizes.data();
        descriptorPoolInfo.maxSets = _maxSets;
        descriptorPoolInfo.flags = _poolFlags;

        if (vkCreateDescriptorPool(m_device.device(), &descriptorPoolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) 
            throw std::runtime_error("failed to create descriptor pool!");
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(m_device.device(), m_descriptorPool, nullptr);
    }

    bool DescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout _descriptorSetLayout, VkDescriptorSet& _descriptor) const 
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.pSetLayouts = &_descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up.
        if (vkAllocateDescriptorSets(m_device.device(), &allocInfo, &_descriptor) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& _descriptors) const {
        vkFreeDescriptorSets(
            m_device.device(),
            m_descriptorPool,
            static_cast<uint32_t>(_descriptors.size()),
            _descriptors.data());
    }

    void DescriptorPool::resetPool() 
    {
        vkResetDescriptorPool(m_device.device(), m_descriptorPool, 0);
    }


    // *************** Descriptor Writer *********************

    DescriptorWriter::DescriptorWriter(DescriptorSetLayout& _setLayout, DescriptorPool& _pool)
        : m_setLayout(_setLayout), m_pool(_pool) 
    {}

    DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t _binding, VkDescriptorBufferInfo* _bufferInfo) 
    {
        assert(m_setLayout.m_bindings.count(_binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = m_setLayout.m_bindings[_binding];

        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = _binding;
        write.pBufferInfo = _bufferInfo;
        write.descriptorCount = 1;

        m_writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::writeImage(uint32_t _binding, VkDescriptorImageInfo* _imageInfo) 
    {
        assert(m_setLayout.m_bindings.count(_binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = m_setLayout.m_bindings[_binding];

        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = _binding;
        write.pImageInfo = _imageInfo;
        write.descriptorCount = 1;

        m_writes.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(VkDescriptorSet& _set) 
    {
        bool success = m_pool.allocateDescriptorSet(m_setLayout.getDescriptorSetLayout(), _set);
        if (!success) 
            return false;

        overwrite(_set);
        return true;
    }

    void DescriptorWriter::overwrite(VkDescriptorSet& _set) {
        for (auto& write : m_writes) {
            write.dstSet = _set;
        }
        vkUpdateDescriptorSets(m_pool.m_device.device(), m_writes.size(), m_writes.data(), 0, nullptr);
    }
}
