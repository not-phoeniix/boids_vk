#include "src/graphics/mesh.h"

#include "src/utility/check_macros.h"

bool mesh_create(
    const vkb::Device& device,
    const MeshCreateInfo& create_info,
    VkCommandPool command_pool,
    VkQueue graphics_queue,
    Mesh* out_mesh
) {
    out_mesh->num_indices = create_info.num_indices;
    out_mesh->num_vertices = create_info.num_vertices;

    // vertex buffer
    {
        // intermediary buffer so we don't have to always be using a
        //  buffer that is accessible by CPU host (faster that way)
        BufferCreateInfo vert_create_info = {
            .size = create_info.vertex_size * create_info.num_vertices,
            .buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        Buffer staging_buffer;
        BOOL_CHECK(buffer_create(device, vert_create_info, &staging_buffer));
        BOOL_CHECK(buffer_map(&staging_buffer));
        buffer_copy_host(
            &staging_buffer,
            create_info.vertices,
            create_info.vertex_size * create_info.num_vertices
        );
        buffer_unmap(&staging_buffer);

        // actual vertex buffer! can't be accessed directly from CPU
        vert_create_info.buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vert_create_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        BOOL_CHECK(buffer_create(device, vert_create_info, &out_mesh->vertex_buffer));

        // copy from host coherent to device local for performance
        buffer_copy(
            &out_mesh->vertex_buffer,
            &staging_buffer,
            command_pool,
            graphics_queue
        );

        // destroy staging buffer when we're done
        vkDeviceWaitIdle(device);
        buffer_destroy(&staging_buffer);
    }

    // index buffer
    {
        // intermediary buffer so we don't have to always be using a
        //  buffer that is accessible by CPU host (faster that way)
        BufferCreateInfo ind_create_info = {
            .size = create_info.index_size * create_info.num_indices,
            .buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        Buffer staging_buffer;
        BOOL_CHECK(buffer_create(device, ind_create_info, &staging_buffer));
        BOOL_CHECK(buffer_map(&staging_buffer));
        buffer_copy_host(
            &staging_buffer,
            create_info.indices,
            create_info.index_size * create_info.num_indices
        );
        buffer_unmap(&staging_buffer);

        // actual index buffer! can't be accessed directly from CPU
        ind_create_info.buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        ind_create_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        BOOL_CHECK(buffer_create(device, ind_create_info, &out_mesh->index_buffer));

        // copy from host coherent to device local for performance
        buffer_copy(
            &out_mesh->index_buffer,
            &staging_buffer,
            command_pool,
            graphics_queue
        );

        // destroy staging buffer when we're done
        vkDeviceWaitIdle(device);
        buffer_destroy(&staging_buffer);
    }

    return true;
}

void mesh_destroy(Mesh* mesh) {
    buffer_destroy(&mesh->vertex_buffer);
    buffer_destroy(&mesh->index_buffer);
}
