#include "graphics.h"

#include "vertex.h"
#include <vector>
#include <fstream>
#include <string>
#include "data_structs.h"
#include <stdexcept>
#include "program_params.h"

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

static VkShaderModule load_shader(const std::string& path, VkDevice device) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
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

    VkShaderModule shader = nullptr;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shader;
}

namespace Graphics {
    static rt::DestructionQueue destroy_queue;

    static void create_api_cluster(GLFWwindow* window) {
        rt::InstanceCreateInfo instance = {
            .app_name = "boids_vk",
            .app_version = VK_MAKE_VERSION(1, 0, 0),
            .api_version = VK_API_VERSION_1_4,
            .validation_layers = VALIDATION_LAYERS.data(),
            .validation_layer_count = static_cast<uint32_t>(VALIDATION_LAYERS.size()),
        };

        rt::ApiClusterCreateInfo api_info = {
            .instance = instance,
            .window = window
        };

        ApiCluster = std::make_shared<rt::ApiCluster>(api_info);
        destroy_queue.QueueDelete([] { ApiCluster.reset(); });
    }

    static void create_instance_data_buffers() {
        // ~~~ create descriptor pool ~~~

        VkDescriptorPoolSize pool_size = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = FRAMES_IN_FLIGHT
        };
        rt::DescriptorPoolCreateInfo pool_info = {
            .max_sets = FRAMES_IN_FLIGHT,
            .flags = 0,
            .pool_sizes = &pool_size,
            .pool_size_count = 1
        };
        InstanceDescriptorPool = std::make_unique<rt::DescriptorPool>(
            pool_info,
            ApiCluster->get_api_context()
        );
        destroy_queue.QueueDelete([] { InstanceDescriptorPool.reset(); });

        // ~~~ create buffers ~~~

        rt::BufferCreateInfo buffer_info = {
            .size = sizeof(InstanceData) * ProgramParams::BOID_COUNT,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };

        InstanceDataBuffers.resize(FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < InstanceDataBuffers.size(); i++) {
            InstanceDataBuffers[i] = std::make_shared<rt::Buffer>(
                buffer_info,
                ApiCluster->get_api_context()
            );
            InstanceDataBuffers[i]->Map();
        }
        destroy_queue.QueueDelete([] { InstanceDataBuffers.clear(); });

        // ~~~ create descriptor set layout ~~~

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            (VkDescriptorSetLayoutBinding) {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
        };

        rt::DescriptorSetLayoutCreateInfo layout_create_info = {
            .bindings = bindings.data(),
            .binding_count = static_cast<uint32_t>(bindings.size()),
        };

        InstanceDataBufferLayout = std::make_shared<rt::DescriptorSetLayout>(
            layout_create_info,
            ApiCluster->get_api_context()
        );
        destroy_queue.QueueDelete([] { InstanceDataBufferLayout.reset(); });

        // ~~~ allocate descriptor sets for buffers ~~~

        InstanceDescriptorSets.resize(FRAMES_IN_FLIGHT);

        // vector full of identical layouts
        std::vector<VkDescriptorSetLayout> set_layouts(
            FRAMES_IN_FLIGHT,
            InstanceDataBufferLayout->get_layout()
        );
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = InstanceDescriptorPool->get_pool(),
            .descriptorSetCount = FRAMES_IN_FLIGHT,
            .pSetLayouts = set_layouts.data()
        };

        VkResult result = vkAllocateDescriptorSets(
            ApiCluster->get_device(),
            &alloc_info,
            InstanceDescriptorSets.data()
        );
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate instance data descriptor set!");
        }

        // ~~~ write to descriptor sets ~~~

        std::vector<VkWriteDescriptorSet> writes(FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < InstanceDescriptorSets.size(); i++) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = InstanceDataBuffers[i]->get_buffer(),
                .offset = 0,
                .range = InstanceDataBuffers[i]->get_size()
            };

            writes[i] = (VkWriteDescriptorSet) {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = InstanceDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = nullptr
            };
        }

        vkUpdateDescriptorSets(
            ApiCluster->get_device(),
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }

    static void create_graphics_manager(GLFWwindow* window, uint32_t width, uint32_t height) {
        // === PIPELINE STUFF =============================

        // ~~~ shaders ~~~

        VkShaderModule vert = load_shader("shaders/vertex.spv", ApiCluster->get_device());
        VkShaderModule frag = load_shader("shaders/pixel.spv", ApiCluster->get_device());

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
            InstanceDataBufferLayout->get_layout(),
        };
        VkPipelineLayoutCreateInfo layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data()
        };

        // ~~~ create pipeline itself ~~~

        rt::GraphicsPipelineCreateInfo pipeline_create_info = {
            .shader_stages = shader_stages.data(),
            .shader_stage_count = static_cast<uint32_t>(shader_stages.size()),
            .vertex_input = &vertex_input_create_info,
            .input_assembly = &input_assembly_create_info,
            .viewport = &viewport_create_info,
            .rasterizer = &rasterizer_create_info,
            .multisample = &multisample_create_info,
            .depth_stencil = &depth_stencil_create_info,
            .color_blend = &color_blend_create_info,
            .dynamic_state = &dynamic_state_create_info,
            .layout_create_info = &layout_create_info,
            // we dont need a render pass cuz the manager will set that up for us
            .render_pass = nullptr,
            .subpass_index = 0
        };

        // === NON PIPELINE STUFF =========================

        rt::SwapChainCreateInfo swap_chain = {
            .frame_flight_count = FRAMES_IN_FLIGHT,
            .depth_format = VK_FORMAT_D32_SFLOAT,
            .extent = (VkExtent2D) {width, height},
        };

        rt::GraphicsManagerCreateInfo manager_info = {
            .clear_value = (VkClearValue) {{0.0f, 0.0f, 0.0f}},
            .window = window,
            .api_cluster = ApiCluster,
            .swap_chain = swap_chain,
            .graphics_pipeline = pipeline_create_info
        };

        Manager = std::make_shared<rt::GraphicsManager>(manager_info);
        GraphicsPipeline = Manager->get_graphics_pipeline();
        SwapChain = Manager->get_swap_chain();
        destroy_queue.QueueDelete([] {
            SwapChain = nullptr;
            GraphicsPipeline = nullptr;
            Manager.reset();
        });

        vkDestroyShaderModule(ApiCluster->get_device(), vert, nullptr);
        vkDestroyShaderModule(ApiCluster->get_device(), frag, nullptr);
    }

    void init(GLFWwindow* window) {
        int width;
        int height;
        glfwGetWindowSize(window, &width, &height);

        create_api_cluster(window);
        create_instance_data_buffers();
        create_graphics_manager(
            window,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        );
    }

    void deinit() {
        destroy_queue.Flush();
    }
}
