#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "vk-bootstrap/VkBootstrap.h"
#include "src/graphics/image.h"

struct FrameSyncData {
    VkSemaphore image_available_semaphore;
    VkFence in_flight_fence;
};

struct Swapchain {
    // device
    vkb::Device device;

    // handles/state stuff
    vkb::Swapchain swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    Image depth_image;
    std::vector<FrameSyncData> per_frame_datas;
    std::vector<VkSemaphore> render_finished_semaphores;
    uint32_t frame_index;
    uint32_t image_index;

    // properties
    uint32_t frames_in_flight;
    bool vsync;
};

struct SwapchainCreateInfo {
    uint32_t frames_in_flight;
    bool vsync;
};

bool swapchain_create(
    const vkb::Device& device,
    const SwapchainCreateInfo& create_info,
    Swapchain* out_swapchain
);
void swapchain_destroy(Swapchain* swapchain);
bool swapchain_recreate(
    Swapchain* out_swapchain,
    const SwapchainCreateInfo* new_create_info = nullptr
);
bool swapchain_wait(Swapchain* swapchain);
void swapchain_next_frame(Swapchain* swapchain);
VkSemaphore swapchain_get_image_available_semaphore(const Swapchain* swapchain);
VkFence swapchain_get_in_flight_fence(const Swapchain* swapchain);
VkSemaphore swapchain_get_render_finished_semaphore(const Swapchain* swapchain);
