#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

VkVertexInputBindingDescription vertex_get_binding_desc();
std::array<VkVertexInputAttributeDescription, 2> vertex_get_attribute_descs();
