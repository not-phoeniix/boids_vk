#pragma once

#include "render_thing/render_thing.h"
#include <memory>

namespace Graphics {
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    inline std::shared_ptr<RenderThing::ApiCluster> ApiCluster = nullptr;
    inline std::shared_ptr<RenderThing::GraphicsPipeline> GraphicsPipeline = nullptr;
    inline std::shared_ptr<RenderThing::GraphicsManager> Manager = nullptr;
    inline std::shared_ptr<RenderThing::SwapChain> SwapChain = nullptr;

    inline std::shared_ptr<RenderThing::DescriptorSetLayout> RingBufferLayout = nullptr;
    inline std::shared_ptr<RenderThing::RingBuffer> RingBuffer = nullptr;

    // sampler stuff

    inline std::shared_ptr<RenderThing::Sampler> Sampler = nullptr;
    inline std::shared_ptr<RenderThing::DescriptorPool> SamplerDescriptorPool = nullptr;
    inline std::shared_ptr<RenderThing::DescriptorSetLayout> SamplerLayout = nullptr;
    inline std::vector<VkDescriptorSet> SamplerDescriptorSets;

    void init(GLFWwindow* window);
    void deinit();

    void write_sampler_image(VkImageView image);
}
