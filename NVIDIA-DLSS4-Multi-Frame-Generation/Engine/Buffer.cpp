/*
 * Encapsulates a Vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

#include "Buffer.h"

// std
#include <cassert>
#include <cstring>

namespace Engine 
{

    /**
     * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
     *
     * @param instanceSize The size of an instance
     * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
     * minUniformBufferOffsetAlignment)
     *
     * @return VkResult of the buffer mapping call
     */
    VkDeviceSize Buffer::getAlignment(VkDeviceSize _instanceSize, VkDeviceSize _minOffsetAlignment) {
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

    /**
     * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
     *
     * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
     * buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the buffer mapping call
     */
    VkResult Buffer::map(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        assert(m_buffer && m_memory && "Called map on buffer before create");
        return vkMapMemory(m_device.device(), m_memory, _offset, _size, 0, &m_mapped);
    }

    /**
     * Unmap a mapped memory range
     *
     * @note Does not return a result as vkUnmapMemory can't fail
     */
    void Buffer::unmap() 
    {
        if (m_mapped)
        {
            vkUnmapMemory(m_device.device(), m_memory);
            m_mapped = nullptr;
        }
    }

    /**
     * Copies the specified data to the mapped buffer. Default value writes whole buffer range
     *
     * @param data Pointer to the data to copy
     * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
     * range.
     * @param offset (Optional) Byte offset from beginning of mapped region
     *
     */
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

    /**
     * Flush a memory range of the buffer to make it visible to the device
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
     * complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the flush call
     */
    VkResult Buffer::flush(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = _offset;
        mappedRange.size = _size;
        return vkFlushMappedMemoryRanges(m_device.device(), 1, &mappedRange);
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
     * the complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the invalidate call
     */
    VkResult Buffer::invalidate(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = _offset;
        mappedRange.size = _size;
        return vkInvalidateMappedMemoryRanges(m_device.device(), 1, &mappedRange);
    }

    /**
     * Create a buffer info descriptor
     *
     * @param size (Optional) Size of the memory range of the descriptor
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkDescriptorBufferInfo of specified offset and range
     */
    VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize _size, VkDeviceSize _offset) 
    {
        return VkDescriptorBufferInfo{
            m_buffer,
            _offset,
            _size,
        };
    }

    /**
     * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
     *
     * @param data Pointer to the data to copy
     * @param index Used in offset calculation
     *
     */
    void Buffer::writeToIndex(void* _data, int _index) { writeToBuffer(_data, m_instanceSize, _index * m_alignmentSize); }

    /**
     *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
     *
     * @param index Used in offset calculation
     *
     */
    VkResult Buffer::flushIndex(int _index) { return flush(m_alignmentSize, _index * m_alignmentSize); }

    /**
     * Create a buffer info descriptor
     *
     * @param index Specifies the region given by index * alignmentSize
     *
     * @return VkDescriptorBufferInfo for instance at index
     */
    VkDescriptorBufferInfo Buffer::descriptorInfoForIndex(int _index) { return descriptorInfo(m_alignmentSize, _index * m_alignmentSize); }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param index Specifies the region to invalidate: index * alignmentSize
     *
     * @return VkResult of the invalidate call
     */
    VkResult Buffer::invalidateIndex(int _index) { return invalidate(m_alignmentSize, _index * m_alignmentSize); }

}