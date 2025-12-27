#pragma once

#include <vulkan/vulkan.h>

struct BufferWrapperCreateInfo {
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

class BufferWrapper {
   private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory device_memory;
    VkDeviceSize size;
    VkBufferUsageFlags buffer_usage;
    VkMemoryPropertyFlags memory_properties;

   public:
    BufferWrapper(const BufferWrapperCreateInfo& create_info, VkDevice device, VkPhysicalDevice physical_device);
    ~BufferWrapper();

    void CopyFromHost(const void* data, size_t size);
    void CopyFromBuffer(const BufferWrapper& src, VkDeviceSize size, VkCommandPool command_pool, VkQueue queue);

    VkBuffer get_buffer() const {
        return buffer;
    }
    VkDeviceSize get_size() const { return size; }
};
