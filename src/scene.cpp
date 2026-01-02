#include "scene.h"

#include "vertex.h"
#include <vulkan/vulkan.h>
#include "input.h"
#include <string>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

constexpr float MOVE_SPEED = 10.0f;
constexpr float SPRINT_SCALAR = 4.0f;
constexpr float LOOK_SPEED = 0.007f;
constexpr uint32_t OBJECT_COUNT = 512;
constexpr float SPAWN_BOX_SIZE = 10.0f;

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

static std::string vec3_to_str(const glm::vec3& vec) {
    return "[" +
           std::to_string(vec.x) + ", " +
           std::to_string(vec.y) + ", " +
           std::to_string(vec.z) + "]";
}

static float randf_range(float min, float max) {
    return min + ((max - min) * ((rand() / (float)RAND_MAX)));
}

void Scene::Init(GraphicsManager& graphics) {
    mesh = make_box_mesh(graphics);

    camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 5.0f, -10.0f),
        graphics.get_aspect(),
        glm::radians(60.0f),
        0.01f,
        1000.0f
    );
    camera->LookAt(glm::vec3(0.0f));

    world_matrices.resize(OBJECT_COUNT);
    colors.resize(OBJECT_COUNT);
    uniforms.resize(OBJECT_COUNT);
    for (uint32_t i = 0; i < OBJECT_COUNT; i++) {
        glm::vec3 pos = {
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f),
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f),
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f)
        };
        pos *= SPAWN_BOX_SIZE;
        glm::vec3 rot = {
            randf_range(0.0f, M_2_PI),
            randf_range(0.0f, M_2_PI),
            randf_range(0.0f, M_2_PI)
        };

        glm::mat4 world = glm::translate(glm::mat4(1.0f), pos);
        world = glm::rotate(world, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        world = glm::rotate(world, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        world = glm::rotate(world, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        world_matrices[i] = world;

        colors[i] = {
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
        };

        uniforms[i] = graphics.MakeNewUniform();
    }
}

void Scene::Deinit() {
    // delete shared ptrs ! call deconstructors !
    mesh.reset();
    camera.reset();
    world_matrices.clear();
    colors.clear();
    uniforms.clear();
}

void Scene::Update(float dt) {
    time += dt;

    // update camera <3

    glm::vec3 move = Input::get_move_axis();

    glm::vec3 offset(0.0f);
    offset += camera->get_forward() * dt * MOVE_SPEED * move.z;
    offset += camera->get_right() * dt * MOVE_SPEED * move.x;
    offset += glm::vec3(0.0f, 1.0f, 0.0f) * dt * MOVE_SPEED * move.y;
    if (Input::get_is_sprinting()) {
        offset *= SPRINT_SCALAR;
    }
    camera->MoveBy(offset);

    glm::vec2 mouse = Input::get_mouse_delta();
    glm::vec3 look(mouse.y * LOOK_SPEED, mouse.x * LOOK_SPEED, 0.0f);
    if (Input::get_lmb_down()) {
        camera->RotateBy(look);
    }
}

void Scene::Draw(GraphicsManager& graphics) {
    camera->set_aspect(graphics.get_aspect());

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

    glm::vec3 ambient(0.005f);

    for (uint32_t i = 0; i < OBJECT_COUNT; i++) {
        UniformBufferObject ubo = {
            .world = world_matrices[i],
            .view = camera->get_view(),
            .proj = camera->get_proj(),
            .color = colors[i],
            .ambient = ambient
        };
        uniforms[i]->CopyData(ubo);
        graphics.CmdBindUniform(uniforms[i]);

        vkCmdDrawIndexed(
            graphics.get_command_buffer(),
            mesh->get_num_indices(),
            1,
            0,
            0,
            0
        );
    }
}
