#include "scene.h"

#include <vector>
#include "vertex.h"
#include <vulkan/vulkan.h>

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
    0,
    1,
    2
};

void Scene::Init(const GraphicsManager& graphics) {
    MeshCreateInfo mesh_info = {
        .vertices = vertices.data(),
        .num_vertices = static_cast<uint32_t>(vertices.size()),
        .indices = indices.data(),
        .num_indices = static_cast<uint32_t>(indices.size())
    };
    mesh = std::make_shared<Mesh>(mesh_info, graphics);
}

void Scene::Deinit() {
    // delete mesh <3
    mesh.reset();
}

void Scene::Update(float dt) {
}

void Scene::Draw(const GraphicsManager& graphics) {
    VkDeviceSize offset = 0;
    VkBuffer vertex_buffer = mesh->get_vertex_buffer();
    vkCmdBindVertexBuffers(
        graphics.get_command_buffer(),
        0,
        1,
        &vertex_buffer,
        &offset
    );

    vkCmdBindIndexBuffer(
        graphics.get_command_buffer(),
        mesh->get_index_buffer(),
        0,
        VK_INDEX_TYPE_UINT32
    );

    vkCmdDrawIndexed(
        graphics.get_command_buffer(),
        mesh->get_num_indices(),
        1,
        0,
        0,
        0
    );
}
