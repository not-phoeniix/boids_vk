#pragma once

#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>

struct CameraPushConstants {
    glm::mat4x4 view;
    glm::mat4x4 proj;
};

struct PixelPushConstants {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 camera_pos;
};
