#pragma once

#include "render_thing/render_thing.h"
#include <memory>

namespace Graphics {
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    constexpr uint32_t MAX_DESCRIPTORS = 64;

    inline std::shared_ptr<rt::ApiCluster> ApiCluster = nullptr;
    inline std::shared_ptr<rt::GraphicsPipeline> GraphicsPipeline = nullptr;
    inline std::shared_ptr<rt::GraphicsManager> Manager = nullptr;
    inline std::shared_ptr<rt::SwapChain> SwapChain = nullptr;

    inline std::shared_ptr<rt::DescriptorPool> DescriptorPool = nullptr;
    inline std::shared_ptr<rt::DescriptorSetLayout> DescriptorSetLayout = nullptr;
    inline std::vector<VkDescriptorSet> DescriptorSets;

    inline std::vector<std::shared_ptr<rt::Buffer>> InstanceDataBuffers;
    inline std::vector<std::shared_ptr<rt::Buffer>> LightBuffers;

    void init(GLFWwindow* window);
    void deinit();
}
