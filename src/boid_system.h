#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include <unordered_set>

using BoidChunk = std::unordered_set<uint32_t>;

struct BoidSystem {
    glm::vec3* boid_positions;
    glm::vec3* boid_velocities;
    glm::vec3* boid_wander_angles;
    uint32_t* boid_contained_chunk_index;
    float* boid_max_speeds;
    uint32_t boid_count;

    BoidChunk* chunks;
    float bounds_size;
};

void boid_system_populate(BoidSystem* system, uint32_t count, float bounds_size);
void boid_system_destroy(BoidSystem* system);
void boid_system_update(BoidSystem* system, float dt);
