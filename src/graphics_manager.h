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
    GLFWwindow* window;

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
    bool framebuffer_resized = false;

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
    void CreateSurface();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    void CleanupSwapChain();
    void RecreateSwapChain();

   public:
    GraphicsManager(GLFWwindow* window);
    ~GraphicsManager();

    void Begin();
    void EndAndPresent();

    // getters/setters

    VkCommandBuffer get_command_buffer() const { return command_buffers[frame_flight_index]; }
    VkDevice get_device() const { return device; }
    VkPhysicalDevice get_physical_device() const { return physical_device; }
    VkClearValue get_clear_value() const { return clear_value; }
    void set_clear_value(VkClearValue clear_value) { this->clear_value = clear_value; }
    void mark_resized() { framebuffer_resized = true; }
};
