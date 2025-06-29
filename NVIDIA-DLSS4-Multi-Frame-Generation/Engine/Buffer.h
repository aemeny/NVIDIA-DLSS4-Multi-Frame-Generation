#pragma once
#include "EngineDevice.h"

namespace Engine {

    struct Buffer
    {
        Buffer(
            EngineDevice& _device,
            VkDeviceSize _instanceSize,
            uint32_t _instanceCount,
            VkBufferUsageFlags _usageFlags,
            VkMemoryPropertyFlags _memoryPropertyFlags,
            VkDeviceSize _minOffsetAlignment = 1);
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        VkResult map(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
        void unmap();

        void writeToBuffer(void* _data, VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
        VkResult flush(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
        VkResult invalidate(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);

        void writeToIndex(void* _data, int _index);
        VkResult flushIndex(int _index);
        VkDescriptorBufferInfo descriptorInfoForIndex(int _index);
        VkResult invalidateIndex(int _index);

        VkBuffer getBuffer() const { return m_buffer; }
        void* getMappedMemory() const { return m_mapped; }
        uint32_t getInstanceCount() const { return m_instanceCount; }
        VkDeviceSize getInstanceSize() const { return m_instanceSize; }
        VkDeviceSize getAlignmentSize() const { return m_instanceSize; }
        VkBufferUsageFlags getUsageFlags() const { return m_usageFlags; }
        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
        VkDeviceSize getBufferSize() const { return m_bufferSize; }

    private:
        static VkDeviceSize getAlignment(VkDeviceSize _instanceSize, VkDeviceSize _minOffsetAlignment);

        EngineDevice& m_device;
        void* m_mapped = nullptr;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkDeviceSize m_bufferSize;
        uint32_t m_instanceCount;
        VkDeviceSize m_instanceSize;
        VkDeviceSize m_alignmentSize;
        VkBufferUsageFlags m_usageFlags;
        VkMemoryPropertyFlags m_memoryPropertyFlags;
    };
}
