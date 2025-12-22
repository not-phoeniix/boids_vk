#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class GraphicsManager {
   private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;

    void CreateInstance();
    void PickPhysicalDevice();

   public:
    GraphicsManager();
    ~GraphicsManager();
};
