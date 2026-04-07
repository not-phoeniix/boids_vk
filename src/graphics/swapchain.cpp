#include "src/graphics/swapchain.h"

#include <memory>
#include "src/utility/utility.h"

bool swapchain_create(
    const vkb::Device& device,
    const SwapchainCreateInfo& create_info,
    Swapchain* out_swapchain
) {
    out_swapchain->device = device;

    // create swapchain & associated data
    auto sc_result =
        vkb::SwapchainBuilder(device)
            .set_desired_present_mode(
                (create_info.vsync)
                    ? VK_PRESENT_MODE_FIFO_KHR
                    : VK_PRESENT_MODE_MAILBOX_KHR
            )
            .set_desired_format({
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            })
            .set_desired_min_image_count(create_info.frames_in_flight + 1)
            .build();
    VKB_CHECK(sc_result);
    out_swapchain->swapchain = sc_result.value();
    out_swapchain->frames_in_flight = create_info.frames_in_flight;
    out_swapchain->vsync = create_info.vsync;
    out_swapchain->frame_index = 0;
    out_swapchain->image_index = 0;

    // save images and image views
    auto images_result = out_swapchain->swapchain.get_images();
    VKB_CHECK(images_result);
    out_swapchain->images = images_result.value();
    auto image_views_result = out_swapchain->swapchain.get_image_views();
    VKB_CHECK(image_views_result);
    out_swapchain->image_views = image_views_result.value();

    // create depth image
    ImageCreateInfo depth_image_info = {
        .format = VK_FORMAT_D32_SFLOAT,
        .extent = out_swapchain->swapchain.extent,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .view_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT
    };
    BOOL_CHECK(image_create(
        device,
        depth_image_info,
        &out_swapchain->depth_image
    ));

    // create frame datas
    {
        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            // we create signaled so the very first frame doesn't lock up lol
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        out_swapchain->per_frame_datas.resize(create_info.frames_in_flight);
        for (size_t i = 0; i < out_swapchain->per_frame_datas.size(); i++) {
            VK_CHECK(vkCreateSemaphore(
                device,
                &semaphore_create_info,
                nullptr,
                &out_swapchain->per_frame_datas[i].image_available_semaphore
            ));

            VK_CHECK(vkCreateFence(
                device,
                &fence_create_info,
                nullptr,
                &out_swapchain->per_frame_datas[i].in_flight_fence
            ));
        }
    }

    // create render finished semaphores
    {
        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        // we make one semaphore for every single swap
        //   chain image rather than each frame in flight
        out_swapchain->render_finished_semaphores.resize(
            out_swapchain->swapchain.image_count
        );
        for (size_t i = 0; i < out_swapchain->render_finished_semaphores.size(); i++) {
            VK_CHECK(vkCreateSemaphore(
                device,
                &semaphore_create_info,
                nullptr,
                &out_swapchain->render_finished_semaphores[i]
            ));
        }
    }

    return true;
}

void swapchain_destroy(Swapchain* swapchain) {
    vkDeviceWaitIdle(swapchain->device);

    image_destroy(&swapchain->depth_image);
    swapchain->images.clear();
    for (const auto& view : swapchain->image_views) {
        vkDestroyImageView(swapchain->device, view, nullptr);
    }
    swapchain->image_views.clear();

    for (const auto& data : swapchain->per_frame_datas) {
        vkDestroySemaphore(swapchain->device, data.image_available_semaphore, nullptr);
        vkDestroyFence(swapchain->device, data.in_flight_fence, nullptr);
    }
    swapchain->per_frame_datas.clear();

    for (const auto& semaphore : swapchain->render_finished_semaphores) {
        vkDestroySemaphore(swapchain->device, semaphore, nullptr);
    }
    swapchain->render_finished_semaphores.clear();

    vkb::destroy_swapchain(swapchain->swapchain);
}

bool swapchain_recreate(
    Swapchain* out_swapchain,
    const SwapchainCreateInfo* new_create_info
) {
    SwapchainCreateInfo prev_info = {
        .frames_in_flight = out_swapchain->frames_in_flight,
        .vsync = out_swapchain->vsync,
    };
    if (new_create_info == nullptr) {
        new_create_info = &prev_info;
    }

    swapchain_destroy(out_swapchain);
    BOOL_CHECK(swapchain_create(
        out_swapchain->device,
        *new_create_info,
        out_swapchain
    ));

    return true;
}

bool swapchain_wait(Swapchain* swapchain) {
    VkSemaphore image_available_semaphore = swapchain_get_image_available_semaphore(swapchain);
    VkFence in_flight_fence = swapchain_get_in_flight_fence(swapchain);

    vkWaitForFences(
        swapchain->device,
        1,
        &in_flight_fence,
        VK_TRUE,
        UINT64_MAX
    );

    VkResult result = vkAcquireNextImageKHR(
        swapchain->device,
        swapchain->swapchain,
        UINT64_MAX,
        image_available_semaphore,
        nullptr,
        &swapchain->image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        BOOL_CHECK(swapchain_recreate(swapchain));
        return false;
    } else if (result != VK_SUBOPTIMAL_KHR) {
        // we can ignore suboptimal results since that won't crash our program
        VK_CHECK(result);
    }

    // only reset things if we're submitting work
    vkResetFences(swapchain->device, 1, &in_flight_fence);

    return true;
}

void swapchain_next_frame(Swapchain* swapchain) {
    swapchain->frame_index = (swapchain->frame_index + 1) % swapchain->frames_in_flight;
}

VkSemaphore swapchain_get_image_available_semaphore(const Swapchain* swapchain) {
    return swapchain->per_frame_datas[swapchain->frame_index].image_available_semaphore;
}

VkFence swapchain_get_in_flight_fence(const Swapchain* swapchain) {
    return swapchain->per_frame_datas[swapchain->frame_index].in_flight_fence;
}

VkSemaphore swapchain_get_render_finished_semaphore(const Swapchain* swapchain) {
    return swapchain->render_finished_semaphores[swapchain->image_index];
}
