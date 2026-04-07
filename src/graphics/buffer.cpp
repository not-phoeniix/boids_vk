#include "src/graphics/buffer.h"

#include "src/utility/utility.h"

bool buffer_create(
    const vkb::Device& device,
    const BufferCreateInfo& create_info,
    Buffer* out_buffer
) {
    out_buffer->device = device;
    out_buffer->size = create_info.size;
    out_buffer->buffer_usage = create_info.buffer_usage;
    out_buffer->memory_properties = create_info.memory_properties;
    out_buffer->mapped = nullptr;

    VkBufferCreateInfo buffer_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = create_info.size,
        .usage = create_info.buffer_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(
        device,
        &buffer_info,
        nullptr,
        &out_buffer->buffer
    ));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, out_buffer->buffer, &mem_req);

    uint32_t memory_type = 0;
    BOOL_CHECK(
        Utils::find_memory_type(
            mem_req.memoryTypeBits,
            create_info.memory_properties,
            device.physical_device,
            &memory_type
        )
    );

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = memory_type,
    };

    VK_CHECK(vkAllocateMemory(
        device,
        &alloc_info,
        nullptr,
        &out_buffer->memory
    ));

    vkBindBufferMemory(device, out_buffer->buffer, out_buffer->memory, 0);

    return true;
}

void buffer_destroy(Buffer* buffer) {
    vkDestroyBuffer(buffer->device, buffer->buffer, nullptr);
    vkFreeMemory(buffer->device, buffer->memory, nullptr);
}

bool buffer_map(Buffer* buffer, uint64_t offset, size_t size) {
    if (buffer->mapped == nullptr) {
        if (size == SIZE_MAX) {
            size = buffer->size;
        }

        VK_CHECK(vkMapMemory(
            buffer->device,
            buffer->memory,
            offset,
            size,
            0,
            &buffer->mapped
        ));
    }

    return true;
}

void buffer_unmap(Buffer* buffer) {
    if (buffer->mapped != nullptr) {
        vkUnmapMemory(buffer->device, buffer->memory);
        buffer->mapped = nullptr;
    }
}

void buffer_copy(
    Buffer* dst,
    const Buffer* src,
    VkCommandPool command_pool,
    VkQueue queue
) {
    VkCommandBuffer command_buffer = Utils::begin_single_use_commands(dst->device, command_pool);

    VkDeviceSize copy_size = src->size;
    if (copy_size > dst->size) copy_size = dst->size;
    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = copy_size
    };
    vkCmdCopyBuffer(
        command_buffer,
        src->buffer,
        dst->buffer,
        1,
        &copy_region
    );

    Utils::end_single_use_commands(
        command_buffer,
        dst->device,
        queue,
        command_pool
    );
}

void buffer_copy_host(
    Buffer* dst,
    const void* data,
    size_t size,
    uint64_t offset
) {
    // this is an annoying thing to type over
    //   and over again, hence the helper func
    memcpy(
        reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(dst->mapped) + offset),
        data,
        size
    );
}
