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

    void init(GLFWwindow* window);
    void deinit();
}
