#include "mesh.h"

Mesh::Mesh(const MeshCreateInfo& create_info, VkDevice device, VkPhysicalDevice physical_device)
  : device(device),
    num_vertices(create_info.num_vertices),
    num_indices(create_info.num_indices) {

    // vertex buffer
    BufferWrapperCreateInfo vertex_create_info = {
        .size = sizeof(Vertex) * create_info.num_vertices,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    vertex_buffer = std::make_unique<BufferWrapper>(vertex_create_info, device, physical_device);
    vertex_buffer->CopyData(create_info.vertices, sizeof(Vertex) * create_info.num_vertices);

    // index buffer
    BufferWrapperCreateInfo index_create_info = {
        .size = sizeof(uint32_t) * create_info.num_indices,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    index_buffer = std::make_unique<BufferWrapper>(index_create_info, device, physical_device);
    index_buffer->CopyData(create_info.indices, sizeof(uint32_t) * create_info.num_indices);
}
