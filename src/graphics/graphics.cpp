#include "src/graphics/graphics.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include "src/vertex.h"
#include "src/data_structs.h"
#include "src/light.h"
#include "src/graphics/swapchain.h"
#include "src/utility/utility.h"
#include "src/program_params.h"

// === "PRIVATE" STUFF ====================================

namespace Graphics {
    static DestructionQueue destroy_queue;

    static GLFWwindow* window;
    static vkb::Instance instance;
    static VkSurfaceKHR surface;
    static VkPhysicalDevice physical_device;
    static vkb::Device device;
    static uint32_t graphics_queue_family;
    static VkQueue graphics_queue;
    static uint32_t present_queue_family;
    static VkQueue present_queue;

    static VkClearValue clear_value = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    static Swapchain* swapchain;
    static bool should_recreate_swap = false;
    static VkCommandPool command_pool;
    static std::vector<VkCommandBuffer> command_buffers;

    static VkPipelineLayout pipeline_layout;
    static VkPipeline graphics_pipeline;
    static VkViewport viewport;

#pragma region // helpers and callbacks

    static bool load_shader(const std::string& path, VkShaderModule* out_shader) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "ERROR: failed to open file: " << path << std::endl;
            return false;
        }

        size_t file_size = static_cast<size_t>(file.tellg());
        std::vector<char> bytecode(file_size);

        file.seekg(0);
        file.read(bytecode.data(), file_size);

        file.close();

        VkShaderModuleCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = bytecode.size(),
            .pCode = reinterpret_cast<const uint32_t*>(bytecode.data())
        };

        VK_CHECK(vkCreateShaderModule(device, &create_info, nullptr, out_shader));

        return true;
    }

    static void on_window_resize(GLFWwindow* window, int width, int height) {
        should_recreate_swap = true;
    }

#pragma endregion

#pragma region // init functions

    static bool vk_init(GLFWwindow* window) {
        // create instance
        auto i_result = vkb::InstanceBuilder()
                            .set_app_name("debug_mode")
                            .require_api_version(VK_API_VERSION_1_3)
                            .request_validation_layers(ENABLE_VALIDATION_LAYERS)
                            .use_default_debug_messenger()
                            // .enable_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                            // .enable_extensions(glfw_extensions)
                            .build();
        VKB_CHECK(i_result);
        instance = i_result.value();
        destroy_queue.QueueDelete([] {
            vkb::destroy_instance(instance);
        });

        // create window surface
        VK_CHECK(glfwCreateWindowSurface(
            instance,
            window,
            nullptr,
            &surface
        ));
        destroy_queue.QueueDelete([] {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        });

        // pick physical device
        auto pd_result = vkb::PhysicalDeviceSelector(instance)
                             .set_surface(surface)
                             .set_required_features_13({
                                 .synchronization2 = true,
                                 .dynamicRendering = true,
                             })
                             .select();
        if (!pd_result.has_value()) {
            std::cerr << "ERROR: Suitable VkPhysicalDevice could not be selected!\n";
            return false;
        }
        physical_device = pd_result.value();

        // create logical device
        auto d_result = vkb::DeviceBuilder(pd_result.value()).build();
        VKB_CHECK(d_result);
        device = d_result.value();
        destroy_queue.QueueDelete([] {
            vkb::destroy_device(device);
        });

        // grab queue stuff
        auto gq_result = device.get_queue_and_index(vkb::QueueType::graphics);
        VKB_CHECK(gq_result);
        graphics_queue = gq_result.value().first;
        graphics_queue_family = gq_result.value().second;
        auto pq_result = device.get_queue_and_index(vkb::QueueType::present);
        VKB_CHECK(pq_result);
        present_queue = pq_result.value().first;
        present_queue_family = pq_result.value().second;

        // make swapchain
        swapchain = new Swapchain();
        SwapchainCreateInfo swap_info = {
            .frames_in_flight = FRAMES_IN_FLIGHT,
            .vsync = VSYNC,
        };
        BOOL_CHECK(swapchain_create(device, swap_info, swapchain));
        destroy_queue.QueueDelete([] {
            swapchain_destroy(swapchain);
            delete swapchain;
        });

        // set up viewport
        viewport = (VkViewport) {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(swapchain->swapchain.extent.width),
            .height = static_cast<float>(swapchain->swapchain.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        return true;
    }

    static bool create_command_pool_and_buffers() {
        // pool
        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics_queue_family,
        };
        VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));
        destroy_queue.QueueDelete([] {
            vkDestroyCommandPool(device, command_pool, nullptr);
        });

        // buffers
        command_buffers.resize(FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = FRAMES_IN_FLIGHT,
        };
        VK_CHECK(vkAllocateCommandBuffers(
            device,
            &alloc_info,
            command_buffers.data()
        ));

        return true;
    }

    static bool create_descriptor_objects() {
        // ~~~ create descriptor pool ~~~

        std::vector<VkDescriptorPoolSize> pool_sizes = {
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = MAX_DESCRIPTORS
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = MAX_DESCRIPTORS
            },
            (VkDescriptorPoolSize) {
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = MAX_DESCRIPTORS
            },
        };

        VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = MAX_DESCRIPTORS,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &DescriptorPool));
        destroy_queue.QueueDelete([] {
            vkDestroyDescriptorPool(device, DescriptorPool, nullptr);
        });

        // ~~~ create descriptor set layout ~~~

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            // instance data buffer
            (VkDescriptorSetLayoutBinding) {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },

            // light buffer
            (VkDescriptorSetLayoutBinding) {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            },
        };

        VkDescriptorSetLayoutCreateInfo layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(
            device,
            &layout_create_info,
            nullptr,
            &DescriptorSetLayout
        ));
        destroy_queue.QueueDelete([] {
            vkDestroyDescriptorSetLayout(device, DescriptorSetLayout, nullptr);
        });

        // ~~~ allocate descriptor sets ~~~

        DescriptorSets.resize(FRAMES_IN_FLIGHT);

        // vector full of identical layouts
        std::vector<VkDescriptorSetLayout> set_layouts(
            FRAMES_IN_FLIGHT,
            DescriptorSetLayout
        );
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = DescriptorPool,
            .descriptorSetCount = FRAMES_IN_FLIGHT,
            .pSetLayouts = set_layouts.data()
        };

        VK_CHECK(vkAllocateDescriptorSets(
            device,
            &alloc_info,
            DescriptorSets.data()
        ));

        return true;
    }

    static bool create_buffers() {
        // ~~~ create buffers ~~~

        BufferCreateInfo instance_buffer_info = {
            .size = sizeof(glm::mat4x4) * ProgramParams::BOID_COUNT,
            .buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };

        BufferCreateInfo light_buffer_info = {
            .size = sizeof(Light) * MAX_LIGHTS + (sizeof(uint32_t) * 4),
            .buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };

        InstanceDataBuffers.resize(FRAMES_IN_FLIGHT);
        LightBuffers.resize(FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            // make buffers
            BOOL_CHECK(buffer_create(
                device,
                instance_buffer_info,
                &InstanceDataBuffers[i]
            ));
            BOOL_CHECK(buffer_create(
                device,
                light_buffer_info,
                &LightBuffers[i]
            ));

            // map buffers
            BOOL_CHECK(buffer_map(&InstanceDataBuffers[i]));
            BOOL_CHECK(buffer_map(&LightBuffers[i]));
        }
        destroy_queue.QueueDelete([] {
            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
                buffer_destroy(&InstanceDataBuffers[i]);
                buffer_destroy(&LightBuffers[i]);
            }
            InstanceDataBuffers.clear();
            LightBuffers.clear();
        });

        // ~~~ write to descriptor sets ~~~

        std::vector<VkWriteDescriptorSet> writes;
        for (size_t i = 0; i < DescriptorSets.size(); i++) {
            VkDescriptorBufferInfo instance_buffer_info = {
                .buffer = InstanceDataBuffers[i].buffer,
                .offset = 0,
                .range = InstanceDataBuffers[i].size,
            };

            VkDescriptorBufferInfo light_buffer_info = {
                .buffer = LightBuffers[i].buffer,
                .offset = 0,
                .range = LightBuffers[i].size,
            };

            // instance data storage buffer write
            writes.push_back((VkWriteDescriptorSet) {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &instance_buffer_info,
                .pTexelBufferView = nullptr
            });

            // light uniform buffer write
            writes.push_back((VkWriteDescriptorSet) {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &light_buffer_info,
                .pTexelBufferView = nullptr
            });
        }

        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );

        return true;
    }

    static bool create_pipeline() {
        // === PIPELINE STUFF =============================

        // ~~~ shaders ~~~

        VkShaderModule vert;
        BOOL_CHECK(load_shader("shaders/vertex.spv", &vert));
        VkShaderModule frag;
        BOOL_CHECK(load_shader("shaders/pixel.spv", &frag));

        VkPipelineShaderStageCreateInfo vert_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert,
            .pName = "main"
        };
        VkPipelineShaderStageCreateInfo frag_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag,
            .pName = "main"
        };

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
            vert_info,
            frag_info
        };

        // ~~~ dynamic states ~~~

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data()
        };

        // ~~~ vertex input ~~~

        auto binding_desc = vertex_get_binding_desc();
        auto attribute_descs = vertex_get_attribute_descs();

        VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &binding_desc,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descs.size()),
            .pVertexAttributeDescriptions = attribute_descs.data()
        };

        // ~~~ input assembly ~~~

        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        // ~~~ viewports & scissors ~~~

        // viewports/scissors are created dynamically!
        //   as per dynamic states defined above <3
        //   meaning we don't have to actually specify
        //   a viewport/scissor on pipeline creation
        VkPipelineViewportStateCreateInfo viewport_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        };

        // ~~~ rasterizer ~~~

        VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f
        };

        // ~~~ multisample ~~~

        VkPipelineMultisampleStateCreateInfo multisample_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE
        };

        // ~~~ depth/stencil ~~~

        VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            // optional depth bounds testing (disabled here)
            .depthBoundsTestEnable = VK_FALSE,
            // optional stencil testing (disabled here)
            .stencilTestEnable = VK_FALSE,
            // parameters for above mentioned testing
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };

        // ~~~ color blending ~~~

        VkPipelineColorBlendAttachmentState alpha_blend_attachment = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT
        };

        VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &alpha_blend_attachment
        };

        // ~~~ PIPELINE LAYOUT (important) ~~~

        std::array<VkPushConstantRange, 2> push_constant_ranges = {
            (VkPushConstantRange) {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(CameraPushConstants)
            },
            (VkPushConstantRange) {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = (sizeof(CameraPushConstants) / 16) * 16,
                .size = sizeof(PixelPushConstants)
            }
        };

        std::vector<VkDescriptorSetLayout> layouts = {
            DescriptorSetLayout,
        };
        VkPipelineLayoutCreateInfo layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data()
        };

        VK_CHECK(vkCreatePipelineLayout(
            device,
            &layout_create_info,
            nullptr,
            &pipeline_layout
        ));
        destroy_queue.QueueDelete([] {
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        });

        // ~~~ rendering info ~~~

        VkPipelineRenderingCreateInfo rendering_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain->swapchain.image_format,
            .depthAttachmentFormat = swapchain->depth_image.format,
        };

        // ~~~ create pipeline itself ~~~

        VkGraphicsPipelineCreateInfo pipeline_create_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering_create_info,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .pVertexInputState = &vertex_input_create_info,
            .pInputAssemblyState = &input_assembly_create_info,
            .pViewportState = &viewport_create_info,
            .pRasterizationState = &rasterizer_create_info,
            .pMultisampleState = &multisample_create_info,
            .pDepthStencilState = &depth_stencil_create_info,
            .pColorBlendState = &color_blend_create_info,
            .pDynamicState = &dynamic_state_create_info,
            .layout = pipeline_layout,
        };

        VK_CHECK(vkCreateGraphicsPipelines(
            device,
            nullptr,
            1,
            &pipeline_create_info,
            nullptr,
            &graphics_pipeline
        ));
        destroy_queue.QueueDelete([] {
            vkDestroyPipeline(device, graphics_pipeline, nullptr);
        });

        // we should destroy our shader modules when
        //   we're done initializing the pipeline lol
        vkDestroyShaderModule(device, vert, nullptr);
        vkDestroyShaderModule(device, frag, nullptr);

        return true;
    }

#pragma endregion

#pragma region // render functions

    static bool scene_render(std::function<void()> scene_render_proc) {
        VkCommandBuffer command_buffer = get_frame_command_buffer();

        // ~~~ begin rendering ~~~

        VkRenderingAttachmentInfo color_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain->image_views[swapchain->image_index],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clear_value,
        };

        VkRenderingAttachmentInfo depth_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain->depth_image.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {.depthStencil = {1.0f, 0}},
        };

        VkRenderingInfo render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .offset = {0, 0},
                .extent = swapchain->swapchain.extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = &depth_attachment_info,
        };

        vkCmdBeginRendering(command_buffer, &render_info);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        // ~~~ set up dynamic state stuff ~~~

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(swapchain->swapchain.extent.width);
        viewport.height = static_cast<float>(swapchain->swapchain.extent.height);
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
            .extent = {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)},
        };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        scene_render_proc();

        vkCmdEndRendering(command_buffer);

        return true;
    }

#pragma endregion
}

// === HEADER DEFINITIONS =================================

bool Graphics::init(GLFWwindow* window) {
    Graphics::window = window;

    BOOL_CHECK(vk_init(window));
    BOOL_CHECK(create_command_pool_and_buffers());
    BOOL_CHECK(create_descriptor_objects());
    BOOL_CHECK(create_buffers());
    BOOL_CHECK(create_pipeline());

    glfwSetWindowSizeCallback(window, on_window_resize);

    return true;
}

void Graphics::deinit() {
    destroy_queue.Flush();
}

bool Graphics::render_frame(std::function<void()> scene_render_proc) {
    // variables we'll be using throughout
    VkCommandBuffer command_buffer = get_frame_command_buffer();
    VkSemaphore image_available_semaphore = swapchain_get_image_available_semaphore(swapchain);
    VkFence in_flight_fence = swapchain_get_in_flight_fence(swapchain);
    VkSemaphore render_finished_semaphore = nullptr;

    // ~~~ RESET LAST FRAME, BEGIN CB ~~~
    {
        BOOL_CHECK(swapchain_wait(swapchain));

        render_finished_semaphore = swapchain_get_render_finished_semaphore(swapchain);

        VkCommandBuffer command_buffer = get_frame_command_buffer();

        vkResetCommandBuffer(command_buffer, 0);

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };
        VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));
    }

    // ~~~ ACTUAL DRAWING ~~~
    {
        // transition from previous frame's layout of "idc whatevs"
        //   to desired layout of "we wanna write to this now !"
        Utils::transition_image_layout(
            swapchain->images[swapchain->image_index],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT,
            command_buffer
        );

        // transition depth image too !
        Utils::transition_image_layout(
            swapchain->depth_image.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            command_buffer
        );

        BOOL_CHECK(scene_render(scene_render_proc));

        // transition from output of writing to color
        //   attachment to the layout of "ready to present"
        // (we actually dont care ab the depth after rendering since
        //   that's sampled and not presented)
        Utils::transition_image_layout(
            swapchain->images[swapchain->image_index],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_ASPECT_COLOR_BIT,
            command_buffer
        );
    }

    // ~~~ END COMMAND BUFFER AND SUBMIT ~~~
    {
        // end command buffer recording before submitting to queue
        VK_CHECK(vkEndCommandBuffer(command_buffer));

        // ~~~ submitting queue ~~~

        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            // image available semaphores are linked to frame flight .
            //   once it's available we will use swapchain and its image
            //   index with inner systems
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = wait_stages,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            // we use more semaphores here! one for each swap
            //  chain image to keep them entirely separate
            .pSignalSemaphores = &render_finished_semaphore,
        };

        VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence));
    }

    // ~~~ PRESENT ~~~
    {
        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain->swapchain.swapchain,
            .pImageIndices = &swapchain->image_index,
            .pResults = nullptr
        };

        VkResult result = vkQueuePresentKHR(present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || should_recreate_swap) {
            should_recreate_swap = false;
            BOOL_CHECK(swapchain_recreate(swapchain));
        } else {
            VK_CHECK(result);
        }
    }

    swapchain_next_frame(swapchain);

    return true;
}

VkInstance Graphics::get_instance() { return instance; }
VkSurfaceKHR Graphics::get_surface() { return surface; }
VkPhysicalDevice Graphics::get_physical_device() { return physical_device; }
const vkb::Device& Graphics::get_device() { return device; }
uint32_t Graphics::get_graphics_queue_family() { return graphics_queue_family; }
VkQueue Graphics::get_graphics_queue() { return graphics_queue; }
VkCommandBuffer Graphics::get_frame_command_buffer() {
    return command_buffers[swapchain->frame_index];
}
VkCommandPool Graphics::get_command_pool() { return command_pool; }
VkPipeline Graphics::get_graphics_pipeline() { return graphics_pipeline; }
VkPipelineLayout Graphics::get_graphics_pipeline_layout() { return pipeline_layout; }
float Graphics::get_aspect() {
    return swapchain->swapchain.extent.width /
           static_cast<float>(swapchain->swapchain.extent.height);
}
uint32_t Graphics::get_frame_index() { return swapchain->frame_index; }
