#pragma once

#include "render_thing/render_thing.h"
#include <memory>

namespace Graphics {
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    inline std::shared_ptr<rt::ApiCluster> ApiCluster = nullptr;
    inline std::shared_ptr<rt::GraphicsPipeline> GraphicsPipeline = nullptr;
    inline std::shared_ptr<rt::GraphicsManager> Manager = nullptr;
    inline std::shared_ptr<rt::SwapChain> SwapChain = nullptr;

    inline std::vector<std::shared_ptr<rt::Buffer>> InstanceDataBuffers;
    inline std::shared_ptr<rt::DescriptorPool> InstanceDescriptorPool = nullptr;
    inline std::shared_ptr<rt::DescriptorSetLayout> InstanceDataBufferLayout = nullptr;
    inline std::vector<VkDescriptorSet> InstanceDescriptorSets;

    inline std::shared_ptr<rt::Sampler> Sampler = nullptr;
    inline std::shared_ptr<rt::DescriptorPool> SamplerDescriptorPool = nullptr;
    inline std::shared_ptr<rt::DescriptorSetLayout> SamplerLayout = nullptr;
    inline std::vector<VkDescriptorSet> SamplerDescriptorSets;

    void init(GLFWwindow* window);
    void deinit();

    void write_sampler_image(VkImageView image);
}
