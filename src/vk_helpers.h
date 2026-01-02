#pragma once

#include <vulkan/vulkan.h>
#include <vector>

VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device);
uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device);
VkCommandBuffer begin_single_use_commands(VkCommandPool command_pool, VkDevice device);
void end_single_use_commands(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool command_pool, VkDevice device);
