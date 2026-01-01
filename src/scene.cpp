#include "scene.h"

#include <vector>
#include "vertex.h"
#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

struct TriData {
    uint16_t pos_indices[3];
    uint16_t normal_index;
};

static std::shared_ptr<Mesh> assemble_mesh(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec3>& normals,
    const std::vector<TriData>& triangles,
    GraphicsManager& graphics
) {
    std::vector<Vertex> vertices(triangles.size() * 3);
    std::vector<uint32_t> indices(triangles.size() * 3);

    uint32_t index_counter = 0;
    uint32_t vertex_counter = 0;
    for (size_t i = 0; i < triangles.size(); i++) {
        TriData triangle = triangles[i];

        vertices[vertex_counter] = {positions[triangle.pos_indices[0]], normals[triangle.normal_index]};
        vertex_counter++;
        vertices[vertex_counter] = {positions[triangle.pos_indices[1]], normals[triangle.normal_index]};
        vertex_counter++;
        vertices[vertex_counter] = {positions[triangle.pos_indices[2]], normals[triangle.normal_index]};
        vertex_counter++;

        indices[index_counter] = index_counter;
        index_counter++;
        indices[index_counter] = index_counter;
        index_counter++;
        indices[index_counter] = index_counter;
        index_counter++;
    }

    MeshCreateInfo mesh_info = {
        .vertices = vertices.data(),
        .num_vertices = static_cast<uint32_t>(vertices.size()),
        .indices = indices.data(),
        .num_indices = static_cast<uint32_t>(indices.size())
    };

    return std::make_shared<Mesh>(mesh_info, graphics);
}

static std::shared_ptr<Mesh> make_box_mesh(GraphicsManager& graphics) {
    std::vector<glm::vec3> positions = {
        {-1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, +1.0f},
        {-1.0f, +1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f},
        {-1.0f, +1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, -1.0f, -1.0f},

        {-1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, +1.0f},
        {-1.0f, +1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f},
        {-1.0f, +1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, -1.0f, -1.0f}
    };

    std::vector<glm::vec3> normals = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, -1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f}
    };

    std::vector<TriData> triangles = {
        {{0, 1, 2}, 0},
        {{2, 1, 3}, 0},

        {{2, 3, 4}, 1},
        {{4, 3, 5}, 1},

        {{4, 5, 6}, 2},
        {{6, 5, 7}, 2},

        {{6, 7, 0}, 3},
        {{0, 7, 1}, 3},

        {{1, 7, 3}, 4},
        {{3, 7, 5}, 4},

        {{6, 0, 4}, 5},
        {{4, 0, 2}, 5},
    };

    return assemble_mesh(positions, normals, triangles, graphics);
}

void Scene::Init(GraphicsManager& graphics) {
    mesh = make_box_mesh(graphics);

    camera = std::make_unique<Camera>(
        glm::vec3(5.0f, 5.0f, 5.0f),
        graphics.get_aspect(),
        glm::radians(60.0f),
        0.01f,
        1000.0f
    );

    uniform = graphics.MakeNewUniform();
}

void Scene::Deinit() {
    // delete shared ptrs ! call deconstructors !
    mesh.reset();
    uniform.reset();
}

void Scene::Update(float dt) {
    time += dt;
}

void Scene::Draw(GraphicsManager& graphics) {
    camera->set_aspect(graphics.get_aspect());

    glm::mat4 world = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(45.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    UniformBufferObject ubo = {
        .world = world,
        .view = camera->get_view(),
        .proj = camera->get_proj(),
        .color = glm::vec3(1.0f, 0.05f, 0.1f),
        .ambient = glm::vec3(0.005f)
    };
    uniform->CopyData(ubo);
    graphics.CmdBindUniform(uniform);

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
