#include "buffer_wrapper.h"
#include <stdexcept>
#include <cstring>

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
  : device(device),
    size(create_info.size),
    buffer_usage(create_info.usage),
    memory_properties(create_info.properties) {
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
    vkDeviceWaitIdle(device);

    Unmap();

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}

void BufferWrapper::CopyFromHostAuto(const void* data, size_t size) {
    Map();
    CopyFromHost(data, size);
    Unmap();
}

void BufferWrapper::CopyFromHost(const void* data, size_t size) {
    if (
        (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0 ||
        (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0
    ) {
        throw std::runtime_error("Cannot copy data into a buffer whose memory properties don't include VK_MEMORY_PROPERTY_HOST_COHERENT_BIT!");
    }

    if (mapped == nullptr) {
        throw std::runtime_error("Failed to copy data, buffer was never mapped!");
    }

    memcpy(mapped, data, size);
}

void BufferWrapper::CopyFromBuffer(const BufferWrapper& src, VkDeviceSize size, VkCommandPool command_pool, VkQueue queue) {
    if ((src.buffer_usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) {
        throw std::runtime_error("Cannot copy data from a buffer whose usage doesn't include VK_BUFFER_USAGE_TRANSFER_SRC_BIT!");
    }

    if ((buffer_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
        throw std::runtime_error("Cannot copy data into a buffer whose usage doesn't include VK_BUFFER_USAGE_TRANSFER_DST_BIT!");
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    // make a brand new command buffer for this one command! wow!
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(command_buffer, src.buffer, buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    };

    vkQueueSubmit(queue, 1, &submit_info, nullptr);

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void BufferWrapper::Map() {
    if (mapped == nullptr) {
        vkMapMemory(device, device_memory, 0, size, 0, &mapped);
    }
}

void BufferWrapper::Unmap() {
    if (mapped != nullptr) {
        vkUnmapMemory(device, device_memory);
        mapped = nullptr;
    }
}
