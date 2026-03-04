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

    constexpr uint32_t CHUNKS_PER_AXIS = 8;

    constexpr uint32_t BOID_COUNT = 10000;
    constexpr float BOID_SPAWN_BOX_SIZE = 300.0f;
    const glm::vec3 BOID_COLOR = {0.8f, 0.8f, 0.8f};

    constexpr float BOID_ADJACENT_SEARCH_RADIUS = 20.0f;
    constexpr float BOID_BOUND_LIMIT_STRENGTH = 4.0f;
    constexpr float BOID_FRICTION_COEFF = 2.0f;
    constexpr float BOID_MAX_SPEED_MIN = 10.0f;
    constexpr float BOID_MAX_SPEED_MAX = 50.0f;

    constexpr float BOID_SEPARATE_STRENGTH = 0.4f;
    constexpr float BOID_COHESION_STRENGTH = 3.0f;
    constexpr float BOID_ALIGNMENT_STRENGTH = 3.0f;

    constexpr float BOID_WANDER_STRENGTH = 1.0f;
    constexpr float BOID_WANDER_TIME = 0.4f;
    constexpr float BOID_WANDER_RADIUS = 50.0f;
};
