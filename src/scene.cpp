#include "scene.h"

#include "vertex.h"
#include <vulkan/vulkan.h>
#include "input.h"
#include <string>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

constexpr float MOVE_SPEED = 10.0f;
constexpr float SPRINT_SCALAR = 4.0f;
constexpr float LOOK_SPEED = 0.007f;
constexpr uint32_t OBJECT_COUNT = 512;
constexpr float SPAWN_BOX_SIZE = 10.0f;

// TODO: just use tinyobj loader please GODS PLEASE

#pragma region // mesh creation

struct TriData {
    uint32_t pos_uv_indices[3][2];
    uint32_t normal_index;
};

static std::shared_ptr<Mesh> assemble_mesh(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec2>& uvs,
    const std::vector<TriData>& triangles,
    const GraphicsContext& ctx
) {
    std::vector<Vertex> vertices(triangles.size() * 3);
    std::vector<uint32_t> indices(triangles.size() * 3);

    uint32_t index_counter = 0;
    uint32_t vertex_counter = 0;
    for (size_t i = 0; i < triangles.size(); i++) {
        TriData triangle = triangles[i];

        vertices[vertex_counter] = {
            positions[triangle.pos_uv_indices[0][0]],
            normals[triangle.normal_index],
            uvs[triangle.pos_uv_indices[0][1]]
        };
        vertex_counter++;
        vertices[vertex_counter] = {
            positions[triangle.pos_uv_indices[1][0]],
            normals[triangle.normal_index],
            uvs[triangle.pos_uv_indices[1][1]]
        };
        vertex_counter++;
        vertices[vertex_counter] = {
            positions[triangle.pos_uv_indices[2][0]],
            normals[triangle.normal_index],
            uvs[triangle.pos_uv_indices[2][1]]
        };
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

    return std::make_shared<Mesh>(mesh_info, ctx);
}

static std::shared_ptr<Mesh> make_box_mesh(const GraphicsContext& ctx) {
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

    std::vector<glm::vec2> uvs = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    std::vector<TriData> triangles = {
        {{{0, 0}, {1, 1}, {2, 2}}, 0},
        {{{2, 2}, {1, 1}, {3, 3}}, 0},

        {{{2, 0}, {3, 1}, {4, 2}}, 1},
        {{{4, 2}, {3, 1}, {5, 3}}, 1},

        {{{4, 0}, {5, 1}, {6, 2}}, 2},
        {{{6, 2}, {5, 1}, {7, 3}}, 2},

        {{{6, 0}, {7, 1}, {0, 2}}, 3},
        {{{0, 2}, {7, 1}, {1, 3}}, 3},

        {{{1, 0}, {7, 1}, {3, 2}}, 4},
        {{{3, 2}, {7, 1}, {5, 3}}, 4},

        {{{6, 0}, {0, 1}, {4, 2}}, 5},
        {{{4, 2}, {0, 1}, {2, 3}}, 5},
    };

    return assemble_mesh(positions, normals, uvs, triangles, ctx);
}

#pragma endregion

static std::shared_ptr<ImageWrapper> make_image(const std::string& path, const GraphicsContext& ctx) {
    int width;
    int height;
    int channels;
    stbi_uc* pixels = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load image!");
    }

    ImageWrapperCreateInfo create_info = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .image_data = pixels
    };

    auto image = std::make_shared<ImageWrapper>(create_info, ctx);

    stbi_image_free(pixels);

    return image;
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
    GraphicsContext ctx = graphics.get_context();

    mesh = make_box_mesh(ctx);

    camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 5.0f, -10.0f),
        graphics.get_aspect(),
        glm::radians(60.0f),
        0.01f,
        1000.0f
    );
    camera->LookAt(glm::vec3(0.0f));

    image = make_image("res/dogwho_is_also___rendered.png", ctx);

    SamplerWrapperCreateInfo sampler_create_info = {
        .min_filter = VK_FILTER_LINEAR,
        .mag_filter = VK_FILTER_LINEAR,
        .address_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };
    sampler = std::make_unique<SamplerWrapper>(sampler_create_info, ctx);

    colors.resize(OBJECT_COUNT);
    positions.resize(OBJECT_COUNT);
    rotations.resize(OBJECT_COUNT);
    rot_speeds.resize(OBJECT_COUNT);
    uniforms.resize(OBJECT_COUNT);
    for (uint32_t i = 0; i < OBJECT_COUNT; i++) {
        glm::vec3 pos = {
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f),
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f),
            randf_range(-SPAWN_BOX_SIZE / 2.0f, SPAWN_BOX_SIZE / 2.0f)
        };
        pos *= SPAWN_BOX_SIZE;
        positions[i] = pos;

        glm::vec3 rot = {
            randf_range(0.0f, M_PI * 2),
            randf_range(0.0f, M_PI * 2),
            randf_range(0.0f, M_PI * 2)
        };
        rotations[i] = rot;

        glm::vec3 rot_speed = {
            randf_range(0.0f, M_PI * 2),
            randf_range(0.0f, M_PI * 2),
            randf_range(0.0f, M_PI * 2)
        };
        rot_speeds[i] = rot_speed;

        colors[i] = {
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
        };

        uniforms[i] = graphics.MakeNewUniform(image->get_view(), sampler->get_sampler());
    }
}

void Scene::Deinit() {
    // delete shared ptrs ! call deconstructors !
    mesh.reset();
    image.reset();
    camera.reset();
    positions.clear();
    rotations.clear();
    rot_speeds.clear();
    colors.clear();
    uniforms.clear();
}

static float past_dt = 1.0f;

void Scene::Update(float dt) {
    past_dt = dt;
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
        glm::mat4 world = glm::translate(glm::mat4(1.0f), positions[i]);
        rotations[i] += (rot_speeds[i] * past_dt);
        glm::vec3 rot = rotations[i];
        world = glm::rotate(world, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        world = glm::rotate(world, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        world = glm::rotate(world, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));

        UniformBufferObject ubo = {
            .world = world,
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
