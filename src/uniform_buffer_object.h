#pragma once

#include <glm/glm.hpp>

struct UniformBufferObject {
    glm::mat4x4 world;
    glm::mat4x4 view;
    glm::mat4x4 proj;
    // world inverse transpose matrix
    // glm::mat4x4 wit;

    glm::vec3 color;
    glm::vec3 ambient;
};
