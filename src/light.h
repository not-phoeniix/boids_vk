#pragma once

//! make sure this matches the light definition in
//!   the lighting.slangi file!!

#include "graphics/glm_settings.h"
#include <glm/glm.hpp>

constexpr uint32_t MAX_LIGHTS = 512;

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 position;
    uint32_t type;
    float intensity;
    float range;
    float spot_angle_diff;
};
