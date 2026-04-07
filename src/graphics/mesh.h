#pragma once

#include <vulkan/vulkan.h>
#include "vk-bootstrap/VkBootstrap.h"
#include "src/graphics/buffer.h"

struct Mesh {
    vkb::Device device;

    Buffer vertex_buffer;
    Buffer index_buffer;

    uint32_t num_vertices;
    uint32_t num_indices;
};

struct MeshCreateInfo {
    size_t vertex_size;
    uint32_t num_vertices;
    const void* vertices;
    size_t index_size;
    uint32_t num_indices;
    const void* indices;
};

bool mesh_create(
    const vkb::Device& device,
    const MeshCreateInfo& create_info,
    VkCommandPool command_pool,
    VkQueue graphics_queue,
    Mesh* out_mesh
);
void mesh_destroy(Mesh* mesh);
