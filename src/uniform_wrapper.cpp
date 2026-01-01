#include "uniform_wrapper.h"

#include <stdexcept>

void UniformWrapper::CreateBuffers(uint32_t count, VkDevice device, VkPhysicalDevice physical_device) {
    VkDeviceSize size = sizeof(UniformBufferObject);
    uniform_buffers.resize(count);

    for (uint32_t i = 0; i < count; i++) {
        BufferWrapperCreateInfo create_info = {
            .size = size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        uniform_buffers[i] = std::make_unique<BufferWrapper>(
            create_info,
            device,
            physical_device
        );
        uniform_buffers[i]->Map();
    }
}

void UniformWrapper::CreateDescriptors(uint32_t count, VkDescriptorSetLayout layout, VkDescriptorPool pool) {
    std::vector<VkDescriptorSetLayout> layouts(count, layout);
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts.data()
    };

    descriptor_sets.resize(count);
    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor sets!");
    }

    for (uint32_t i = 0; i < count; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = uniform_buffers[i]->get_buffer(),
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets[i],
            .dstBinding = 0,
            //! descriptors can also be arrays! come back to here when
            //!   you are using instanced rendering for the boids
            //! (this is the starting index to write to)
            // TODO: change this later as WELL so we can pass in a ton of data
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,       // if we had samplers
            .pBufferInfo = &buffer_info, // our buffers we're actually gonna be using
            .pTexelBufferView = nullptr  // if we had other buffer stuff
        };

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

UniformWrapper::UniformWrapper(
    const UniformWrapperCreateInfo& create_info,
    VkDevice device,
    VkPhysicalDevice physical_device
) : device(device),
    frame_flight_count(create_info.frame_flight_count),
    frame_flight_index(0) {
    CreateBuffers(create_info.frame_flight_count, device, physical_device);
    CreateDescriptors(create_info.frame_flight_count, create_info.layout, create_info.pool);
}

UniformWrapper::~UniformWrapper() {
    uniform_buffers.clear();
    descriptor_sets.clear();
}

void UniformWrapper::CopyData(const UniformBufferObject& data) {
    uniform_buffers[frame_flight_index]->CopyFromHost(
        &data,
        sizeof(UniformBufferObject)
    );
}
