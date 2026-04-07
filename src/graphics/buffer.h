#pragma once

#include <vulkan/vulkan.h>
#include "vk-bootstrap/VkBootstrap.h"

struct Buffer {
    vkb::Device device;

    // handles/ptrs
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped;

    // properties
    VkDeviceSize size;
    VkBufferUsageFlags buffer_usage;
    VkMemoryPropertyFlags memory_properties;
};

struct BufferCreateInfo {
    size_t size;
    VkBufferUsageFlags buffer_usage;
    VkMemoryPropertyFlags memory_properties;
};

bool buffer_create(
    const vkb::Device& device,
    const BufferCreateInfo& create_info,
    Buffer* out_buffer
);
void buffer_destroy(Buffer* buffer);
bool buffer_map(Buffer* buffer, uint64_t offset = 0, size_t size = SIZE_MAX);
void buffer_unmap(Buffer* buffer);
void buffer_copy(
    Buffer* dst,
    const Buffer* src,
    VkCommandPool command_pool,
    VkQueue queue
);
void buffer_copy_host(
    Buffer* dst,
    const void* data,
    size_t size,
    uint64_t offset = 0
);
