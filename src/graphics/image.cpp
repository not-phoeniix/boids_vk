#include "src/graphics/image.h"

#include "src/utility/utility.h"
#include "src/graphics/buffer.h"

bool image_create(
    const vkb::Device& device,
    const ImageCreateInfo& create_info,
    Image* out_image
) {
    out_image->device = device;

    // create image itself
    {
        VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = create_info.format,
            .extent = {
                .width = create_info.extent.width,
                .height = create_info.extent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = create_info.tiling,
            .usage = create_info.usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VK_CHECK(vkCreateImage(
            device,
            &image_create_info,
            nullptr,
            &out_image->image
        ));

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(device, out_image->image, &mem_requirements);

        uint32_t memory_type = 0;
        BOOL_CHECK(
            Utils::find_memory_type(
                mem_requirements.memoryTypeBits,
                create_info.memory_properties,
                device.physical_device,
                &memory_type
            )
        );

        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = memory_type
        };
        VK_CHECK(vkAllocateMemory(
            device,
            &alloc_info,
            nullptr,
            &out_image->memory
        ));

        vkBindImageMemory(device, out_image->image, out_image->memory, 0);

        // set properties
        out_image->extent = create_info.extent;
        out_image->format = create_info.format;
    }

    // create image view
    {
        VkImageViewCreateInfo view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = out_image->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.format,
            .subresourceRange = {
                .aspectMask = create_info.view_aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        VK_CHECK(vkCreateImageView(
            device,
            &view_create_info,
            nullptr,
            &out_image->view
        ));
    }

    return true;
}

void image_destroy(Image* image) {
    vkDestroyImage(image->device, image->image, nullptr);
    vkFreeMemory(image->device, image->memory, nullptr);
    vkDestroyImageView(image->device, image->view, nullptr);
}

bool image_copy_data(
    Image* dst,
    const void* data,
    size_t size,
    VkCommandBuffer command_buffer
) {
    BufferCreateInfo staging_create_info = {
        .size = size,
        .buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    Buffer staging_buffer;
    BOOL_CHECK(buffer_create(dst->device, staging_create_info, &staging_buffer));
    BOOL_CHECK(buffer_map(&staging_buffer));
    buffer_copy_host(&staging_buffer, data, size, 0);
    buffer_unmap(&staging_buffer);

    Utils::transition_image_layout(
        dst->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        command_buffer
    );

    Utils::copy_buffer_to_image(
        staging_buffer.buffer,
        dst->image,
        dst->extent,
        command_buffer
    );

    Utils::transition_image_layout(
        dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        command_buffer
    );

    return true;
}
