#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>

class GraphicsManager {
   private:
    VkInstance instance;
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device;
    VkSurfaceKHR surface;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    uint32_t swap_chain_image_index = 0;
    uint32_t frame_flight_index = 0;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;
    VkClearValue clear_value = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    std::vector<VkSemaphore> image_available_sempahores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface(GLFWwindow* window);
    void CreateSwapChain(GLFWwindow* window);
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

   public:
    GraphicsManager(GLFWwindow* window);
    ~GraphicsManager();

    void Begin();
    void EndAndPresent();

    // getters/setters

    VkCommandBuffer& get_command_buffer() { return command_buffers[frame_flight_index]; }
    VkDevice& get_device() { return device; }
    VkClearValue get_clear_value() { return clear_value; }
    void set_clear_value(VkClearValue clear_value) { this->clear_value = clear_value; }
};
