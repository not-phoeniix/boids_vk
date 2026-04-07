#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace Utils {
    VkCommandBuffer begin_single_use_commands(VkDevice device, VkCommandPool command_pool);
    void end_single_use_commands(
        VkCommandBuffer command_buffer,
        VkDevice device,
        VkQueue queue,
        VkCommandPool command_pool
    );

    void transition_image_layout(
        VkImage image,
        VkImageLayout prev_layout,
        VkImageLayout new_layout,
        VkImageAspectFlags aspect,
        VkCommandBuffer command_buffer
    );
    void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        VkExtent2D extent,
        VkCommandBuffer command_buffer
    );
    bool find_memory_type(
        uint32_t type_filter,
        VkMemoryPropertyFlags properties,
        VkPhysicalDevice physical_device,
        uint32_t* out_type
    );
}
