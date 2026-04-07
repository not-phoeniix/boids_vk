#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <functional>
#include "vk-bootstrap/VkBootstrap.h"
#include "src/graphics/buffer.h"

namespace Graphics {
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    constexpr bool VSYNC = true;
    constexpr uint32_t MAX_DESCRIPTORS = 64;
    constexpr uint32_t RING_BUFFER_MAX_ELEMENTS = 1000;
    constexpr bool ENABLE_VALIDATION_LAYERS = true;

    inline VkDescriptorPool DescriptorPool = nullptr;
    inline VkDescriptorSetLayout DescriptorSetLayout = nullptr;
    inline std::vector<VkDescriptorSet> DescriptorSets;
    inline std::vector<Buffer> InstanceDataBuffers;
    inline std::vector<Buffer> LightBuffers;

    bool init(GLFWwindow* window);
    void deinit();

    bool render_frame(std::function<void()> scene_render_proc);

    VkInstance get_instance();
    VkSurfaceKHR get_surface();
    VkPhysicalDevice get_physical_device();
    const vkb::Device& get_device();
    uint32_t get_graphics_queue_family();
    VkQueue get_graphics_queue();
    VkCommandBuffer get_frame_command_buffer();
    VkCommandPool get_command_pool();
    VkPipeline get_graphics_pipeline();
    VkPipelineLayout get_graphics_pipeline_layout();
    float get_aspect();
    uint32_t get_frame_index();
}
