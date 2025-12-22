#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.h>
#include <vector>

class GraphicsManager {
   private:
    VkInstance instance;
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device;

    VkQueue graphics_queue;

    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

   public:
    GraphicsManager();
    ~GraphicsManager();
};
