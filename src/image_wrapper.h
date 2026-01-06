#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

struct ImageWrapperCreateInfo {
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags image_usage;
    VkMemoryPropertyFlags memory_properties;
    const void* image_data;
};

class ImageWrapper {
   private:
    VkDevice device;
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkFormat image_format;
    VkImageLayout image_layout;
    uint32_t width;
    uint32_t height;

    void CreateImage(const ImageWrapperCreateInfo& create_info, const GraphicsContext& ctx);
    void CopyData(const void* data, const GraphicsContext& ctx);
    void CreateImageView(const GraphicsContext& ctx);

   public:
    ImageWrapper(const ImageWrapperCreateInfo& create_info, const GraphicsContext& ctx);
    ~ImageWrapper();

    VkImage get_image() const { return image; }
    VkImageView get_image_view() const { return view; }
    uint32_t get_width() const { return width; }
    uint32_t get_height() const { return height; }
};
