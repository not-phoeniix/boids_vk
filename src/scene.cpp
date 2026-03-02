#include "scene.h"

#include <vulkan/vulkan.h>
#include "input.h"
#include <string>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include "graphics.h"
#include "data_structs.h"
#include "vertex.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace rt;

constexpr float PERFORMANCE_PRINT_INTERVAL = 1.0f;

constexpr float MOVE_SPEED = 10.0f;
constexpr float SPRINT_SCALAR = 4.0f;
constexpr float LOOK_SPEED = 0.007f;
constexpr uint32_t OBJECT_COUNT = 5000;
constexpr float SPAWN_BOX_SIZE = 200.0f;
constexpr glm::vec3 BOID_COLOR = {1.0f, 0.0f, 0.1f};

#pragma region // helpers

static std::shared_ptr<Mesh> load_mesh(const std::string& path, const GraphicsContext& g_ctx, const ApiContext& a_ctx) {
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
        .vertex_size = sizeof(Vertex),
        .num_vertices = static_cast<uint32_t>(vertices.size()),
        .indices = indices.data(),
        .index_size = sizeof(uint32_t),
        .num_indices = static_cast<uint32_t>(indices.size()),
    };
    return std::make_shared<Mesh>(mesh_info, g_ctx, a_ctx);
}

static std::shared_ptr<Image> load_image(
    const std::string& path,
    const GraphicsContext& g_ctx,
    const ApiContext& a_ctx
) {
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

    ImageCreateInfo create_info = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .image_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
    };
    auto image = std::make_shared<Image>(create_info, a_ctx);
    image->CopyData(pixels, g_ctx, a_ctx);

    stbi_image_free(pixels);

    return image;
}

static glm::vec3 get_rot_look_at(const glm::vec3& source, const glm::vec3& target) {
    glm::vec3 delta = target - source;
    if (glm::length2(delta) > FLT_EPSILON) {
        delta = glm::normalize(delta);
    }

    float yaw = atan2f(delta.x, delta.z);
    float pitch = asinf(-delta.y);
    return glm::vec3(pitch, yaw, 0);
}

#pragma endregion

Scene::Scene() {
    ApiContext a_ctx = Graphics::Manager->get_api_context();
    GraphicsContext g_ctx = Graphics::Manager->get_graphics_context();

    mesh = load_mesh("res/cube.obj", g_ctx, a_ctx);

    camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 50.0, -200.0f),
        Graphics::Manager->get_aspect(),
        glm::radians(85.0f),
        0.01f,
        1000.0f
    );
    camera->LookAt(glm::vec3(0.0f));

    image = load_image("res/dogwho_is_also___rendered.png", g_ctx, a_ctx);
    Graphics::write_sampler_image(image->get_view());

    boid_system_populate(&boid_system, OBJECT_COUNT, SPAWN_BOX_SIZE);
}

Scene::~Scene() {
    // delete shared ptrs ! call deconstructors !
    mesh.reset();
    image.reset();
    camera.reset();
    boid_system_destroy(&boid_system);
}

void Scene::Update(float dt) {

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

    // update boid system

    float boid_time_start = glfwGetTime();
    boid_system_update(&boid_system, dt);
    float boid_time_end = glfwGetTime();

    static float boid_time_sum = 0.0f;
    static uint32_t frame_counter = 0;
    boid_time_sum += boid_time_end - boid_time_start;
    frame_counter++;

    time += dt;

    static float print_time_prev = 0;
    if (time >= print_time_prev + PERFORMANCE_PRINT_INTERVAL) {
        print_time_prev = time;

        float avg = boid_time_sum / static_cast<float>(frame_counter);
        std::cout << "boid sim avg time: " << (avg * 1000) << "ms\n";

        boid_time_sum = 0;
        frame_counter = 0;
    }
}

void Scene::Draw() {
    VkCommandBuffer command_buffer = Graphics::Manager->get_command_buffer();
    camera->set_aspect(Graphics::Manager->get_aspect());

    // ~~~ binding vertex/index buffers ~~~

    VkDeviceSize offset = 0;
    VkBuffer vertex_buffer = mesh->get_vertex_buffer();
    vkCmdBindVertexBuffers(
        command_buffer,
        0,
        1,
        &vertex_buffer,
        &offset
    );

    vkCmdBindIndexBuffer(
        command_buffer,
        mesh->get_index_buffer(),
        0,
        VK_INDEX_TYPE_UINT32
    );

    // ~~~ push constants ~~~

    CameraPushConstants camera_data = {
        .view = camera->get_view(),
        .proj = camera->get_proj()
    };
    vkCmdPushConstants(
        command_buffer,
        Graphics::GraphicsPipeline->get_layout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(camera_data),
        &camera_data
    );

    PixelPushConstants pixel_data = {
        .color = BOID_COLOR,
        .ambient = glm::vec3(0.005f)
    };
    vkCmdPushConstants(
        command_buffer,
        Graphics::GraphicsPipeline->get_layout(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(CameraPushConstants), // offset by camera data
        sizeof(pixel_data),
        &pixel_data
    );

    // ~~~ actual drawing code ~~~

    for (uint32_t b = 0; b < boid_system.boid_count; b++) {
        // make matrices
        glm::vec3 pos = boid_system.boid_positions[b];
        glm::vec3 rot = get_rot_look_at(pos, pos + boid_system.boid_velocities[b]);
        glm::mat4 world = glm::translate(glm::mat4(1.0f), pos);
        world = glm::rotate(world, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        world = glm::rotate(world, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        world = glm::rotate(world, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));

        // copy uniform data
        UniformBufferObject ubo = {
            .world = world
        };
        VkDescriptorSet set = Graphics::RingBuffer->CopyToNextRegion(&ubo, sizeof(ubo));
        std::vector<VkDescriptorSet> sets = {
            set,
            Graphics::SamplerDescriptorSets[Graphics::SwapChain->get_frame_index()]
        };
        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            Graphics::GraphicsPipeline->get_layout(),
            0,
            static_cast<uint32_t>(sets.size()),
            sets.data(),
            0,
            nullptr
        );

        // draw boid box
        vkCmdDrawIndexed(
            command_buffer,
            mesh->get_num_indices(),
            1,
            0,
            0,
            0
        );
    }
}
