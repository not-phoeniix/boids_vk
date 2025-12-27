#include "buffer_wrapper.h"
#include <stdexcept>
#include <cstring>

// TODO: staging buffer https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer

static uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find any suitable memory type!");
}

BufferWrapper::BufferWrapper(const BufferWrapperCreateInfo& create_info, VkDevice device, VkPhysicalDevice physical_device)
  : device(device) {
    VkBufferCreateInfo buffer_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = create_info.size,
        .usage = create_info.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(
            mem_req.memoryTypeBits,
            create_info.properties,
            physical_device
        )
    };

    if (vkAllocateMemory(device, &alloc_info, nullptr, &device_memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate GPU buffer memory!");
    }

    vkBindBufferMemory(device, buffer, device_memory, 0);
}

BufferWrapper::~BufferWrapper() {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}

void BufferWrapper::CopyData(const void* data, size_t size) {
    void* data_ptr;
    vkMapMemory(device, device_memory, 0, static_cast<VkDeviceSize>(size), 0, &data_ptr);
    memcpy(data_ptr, data, size);
    vkUnmapMemory(device, device_memory);
}
