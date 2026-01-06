#include "scene.h"

#include "graphics/vertex.h"
#include <vulkan/vulkan.h>
#include "input.h"
#include <string>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

constexpr float MOVE_SPEED = 10.0f;
constexpr float SPRINT_SCALAR = 4.0f;
constexpr float LOOK_SPEED = 0.007f;
constexpr uint32_t OBJECT_COUNT = 512;
constexpr float SPAWN_BOX_SIZE = 10.0f;

// TODO: just use tinyobj loader please GODS PLEASE

#pragma region // helpers

static std::shared_ptr<Mesh> load_mesh(const std::string& path, const GraphicsContext& ctx) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(err);
    }

    uint32_t index_counter = 0;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v = {
                .pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                },
                .normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                },
                .uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1],
                }
            };

            // invert y coordinate in UVs
            //   (0 is top left, not bottom left like OBJ specifies)
            v.uv.y = 1.0f - v.uv.y;

            vertices.push_back(v);
            indices.push_back(index_counter++);
        }
    }

    MeshCreateInfo mesh_info = {
        .vertices = vertices.data(),
        .num_vertices = static_cast<uint32_t>(vertices.size()),
        .indices = indices.data(),
        .num_indices = static_cast<uint32_t>(indices.size())
    };
    return std::make_shared<Mesh>(mesh_info, ctx);
}

static std::shared_ptr<ImageWrapper> load_image(const std::string& path, const GraphicsContext& ctx) {
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
        .view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
    };
    auto image = std::make_shared<ImageWrapper>(create_info, ctx);
    image->CopyData(pixels, ctx);

    stbi_image_free(pixels);

    return image;
}

static float randf_range(float min, float max) {
    return min + ((max - min) * ((rand() / (float)RAND_MAX)));
}

#pragma endregion

void Scene::Init(GraphicsManager& graphics) {
    GraphicsContext ctx = graphics.get_context();

    mesh = load_mesh("res/cube.obj", ctx);

    camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 5.0f, -10.0f),
        graphics.get_aspect(),
        glm::radians(60.0f),
        0.01f,
        1000.0f
    );
    camera->LookAt(glm::vec3(0.0f));

    image = load_image("res/dogwho_is_also___rendered.png", ctx);

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
