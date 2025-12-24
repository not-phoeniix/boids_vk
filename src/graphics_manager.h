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

    VkQueue graphics_queue;
    VkQueue present_queue;

    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface(GLFWwindow* window);
    void CreateSwapChain(GLFWwindow* window);
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();

   public:
    GraphicsManager(GLFWwindow* window);
    ~GraphicsManager();
};
