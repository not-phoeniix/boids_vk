#include "scene.h"

#include <vector>
#include "vertex.h"
#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include "input.h"
#include <string>
#include <iostream>

constexpr float move_speed = 10.0f;
constexpr float sprint_scalar = 2.0f;
constexpr float look_speed = 20.0f;

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

    uniform = graphics.MakeNewUniform();
    uniform_two = graphics.MakeNewUniform();
    uniform_three = graphics.MakeNewUniform();
}

void Scene::Deinit() {
    // delete shared ptrs ! call deconstructors !
    mesh.reset();
    uniform.reset();
    uniform_two.reset();
    uniform_three.reset();
}

void Scene::Update(float dt) {
    time += dt;

    // update camera <3

    glm::vec3 move = Input::get_move_axis();

    glm::vec3 offset(0.0f);
    offset += camera->get_forward() * dt * move_speed * move.z;
    offset += camera->get_right() * dt * move_speed * move.x;
    offset += glm::vec3(0.0f, 1.0f, 0.0f) * dt * move_speed * move.y;
    if (Input::get_is_sprinting()) {
        offset *= sprint_scalar;
    }
    camera->MoveBy(offset);

    glm::vec2 mouse = Input::get_mouse_delta();
    glm::vec3 look(mouse.y * dt * look_speed, mouse.x * dt * look_speed, 0.0f);
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

    // object one
    {
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
            .ambient = ambient
        };
        uniform->CopyData(ubo);
        graphics.CmdBindUniform(uniform);

        vkCmdDrawIndexed(
            graphics.get_command_buffer(),
            mesh->get_num_indices(),
            1,
            0,
            0,
            0
        );
    }

    // object two
    {
        glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.0f, 0.0f));
        world = glm::rotate(
            world,
            time * glm::radians(15.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        UniformBufferObject ubo = {
            .world = world,
            .view = camera->get_view(),
            .proj = camera->get_proj(),
            .color = glm::vec3(0.1f, 0.05f, 1.0f),
            .ambient = ambient
        };
        uniform_two->CopyData(ubo);
        graphics.CmdBindUniform(uniform_two);

        vkCmdDrawIndexed(
            graphics.get_command_buffer(),
            mesh->get_num_indices(),
            1,
            0,
            0,
            0
        );
    }

    // object three
    {
        glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
        world = glm::rotate(
            world,
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        UniformBufferObject ubo = {
            .world = world,
            .view = camera->get_view(),
            .proj = camera->get_proj(),
            .color = glm::vec3(0.1f, 1.0f, 0.05f),
            .ambient = ambient
        };
        uniform_three->CopyData(ubo);
        graphics.CmdBindUniform(uniform_three);

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
