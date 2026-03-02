#pragma once

#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>

struct CameraPushConstants {
    alignas(16) glm::mat4x4 view;
    alignas(16) glm::mat4x4 proj;
};

struct PixelPushConstants {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 ambient;
};

struct UniformBufferObject {
    glm::mat4x4 world;
};
