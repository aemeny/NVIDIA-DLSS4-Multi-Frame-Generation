/*
 * Encapsulates a Vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

#include "Buffer.h"

#include <cassert>
#include <cstring>

namespace Engine 
{
    VkDeviceSize Buffer::getAlignment(VkDeviceSize _instanceSize, VkDeviceSize _minOffsetAlignment) 
    {
        if (_minOffsetAlignment > 0) 
            return (_instanceSize + _minOffsetAlignment - 1) & ~(_minOffsetAlignment - 1);

        return _instanceSize;
    }

    Buffer::Buffer(
        EngineDevice& _device,
        VkDeviceSize _instanceSize,
        uint32_t _instanceCount,
        VkBufferUsageFlags _usageFlags,
        VkMemoryPropertyFlags _memoryPropertyFlags,
        VkDeviceSize _minOffsetAlignment)
        : m_device(_device),
        m_instanceSize(_instanceSize),
        m_instanceCount(_instanceCount),
        m_usageFlags(_usageFlags),
        m_memoryPropertyFlags(_memoryPropertyFlags) 
    {
        m_alignmentSize = getAlignment(_instanceSize, _minOffsetAlignment);
        m_bufferSize = m_alignmentSize * _instanceCount;
        m_device.createBuffer(m_bufferSize, _usageFlags, _memoryPropertyFlags, m_buffer, m_memory);
    }

    Buffer::~Buffer() 
    {
        unmap();
        vkDestroyBuffer(m_device.device(), m_buffer, nullptr);
        vkFreeMemory(m_device.device(), m_memory, nullptr);
    }

    VkResult Buffer::map(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        assert(m_buffer && m_memory && "Called map on buffer before create");
        return vkMapMemory(m_device.device(), m_memory, _offset, _size, 0, &m_mapped);
    }

    void Buffer::unmap() 
    {
        if (m_mapped)
        {
            vkUnmapMemory(m_device.device(), m_memory);
            m_mapped = nullptr;
        }
    }

    void Buffer::writeToBuffer(void* _data, VkDeviceSize _size, VkDeviceSize _offset) 
    {
        assert(m_mapped && "Cannot copy to unmapped buffer");

        if (_size == VK_WHOLE_SIZE) 
        {
            memcpy(m_mapped, _data, m_bufferSize);
        }
        else 
        {
            char* memOffset = (char*)m_mapped;
            memOffset += _offset;
            memcpy(memOffset, _data, _size);
        }
    }

    VkResult Buffer::flush(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = _offset;
        mappedRange.size = _size;
        return vkFlushMappedMemoryRanges(m_device.device(), 1, &mappedRange);
    }

    VkResult Buffer::invalidate(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = _offset;
        mappedRange.size = _size;
        return vkInvalidateMappedMemoryRanges(m_device.device(), 1, &mappedRange);
    }

    VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        return VkDescriptorBufferInfo{m_buffer, _offset, _size};
    }

    void Buffer::writeToIndex(void* _data, int _index) 
    { 
        writeToBuffer(_data, m_instanceSize, _index * m_alignmentSize); 
    }

    VkResult Buffer::flushIndex(int _index) 
    { 
        return flush(m_alignmentSize, _index * m_alignmentSize); 
    }

    VkDescriptorBufferInfo Buffer::descriptorInfoForIndex(int _index) 
    { 
        return descriptorInfo(m_alignmentSize, _index * m_alignmentSize); 
    }

    VkResult Buffer::invalidateIndex(int _index) 
    { 
        return invalidate(m_alignmentSize, _index * m_alignmentSize); 
    }
}