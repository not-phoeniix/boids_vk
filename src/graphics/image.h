#pragma once

#include <vulkan/vulkan.h>
#include "vk-bootstrap/VkBootstrap.h"

struct Image {
    vkb::Device device;

    // handles/ptrs
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;

    // properties
    VkExtent2D extent;
    VkFormat format;
};

struct ImageCreateInfo {
    VkFormat format;
    VkExtent2D extent;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memory_properties;
    VkImageAspectFlags view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

bool image_create(
    const vkb::Device& device,
    const ImageCreateInfo& create_info,
    Image* out_image
);
void image_destroy(Image* image);
bool image_copy_data(
    Image* dst,
    const void* data,
    size_t size,
    VkCommandBuffer command_buffer
);
