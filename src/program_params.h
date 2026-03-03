#pragma once

#include <cstdint>
#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>

namespace ProgramParams {
    constexpr uint32_t WINDOW_WIDTH = 800;
    constexpr uint32_t WINDOW_HEIGHT = 600;

    constexpr float FPS_AVG_INTERVAL = 1.0f;
    constexpr float BOID_PERFORMANCE_INTERVAL = 1.0f;

    constexpr float CAM_MOVE_SPEED = 10.0f;
    constexpr float CAM_SPRINT_SCALAR = 4.0f;
    constexpr float CAM_LOOK_SPEED = 0.007f;

    constexpr uint32_t BOID_COUNT = 5000;
    constexpr float BOID_SPAWN_BOX_SIZE = 200.0f;
    constexpr glm::vec3 BOID_COLOR = {0.8f, 0.8f, 0.8f};
};
