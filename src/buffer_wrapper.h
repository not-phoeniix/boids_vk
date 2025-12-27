#pragma once

#include <vulkan/vulkan.h>

struct BufferWrapperCreateInfo {
    size_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

class BufferWrapper {
   private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory device_memory;

   public:
    BufferWrapper(const BufferWrapperCreateInfo& create_info, VkDevice device, VkPhysicalDevice physical_device);
    ~BufferWrapper();

    void CopyData(const void* data, size_t size);

    VkBuffer get_buffer() const { return buffer; }
};
