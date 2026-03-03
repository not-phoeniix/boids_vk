#pragma once

#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include <unordered_set>
#include "thread_pool.h"
#include "render_thing/render_thing.h"
#include <memory>

using BoidChunk = std::unordered_set<uint32_t>;

struct BoidSystem {
    std::vector<glm::vec3> boid_positions;
    std::vector<glm::vec3> boid_velocities;
    std::vector<glm::vec3> boid_wander_angles;
    std::vector<uint32_t> boid_contained_chunk_index;
    std::vector<float> boid_max_speeds;
    uint32_t boid_count;

    std::vector<BoidChunk> chunks;
    float bounds_size;
    std::shared_ptr<ThreadPool> thread_pool;
};

void boid_system_populate(BoidSystem* system, uint32_t count, float bounds_size);
void boid_system_destroy(BoidSystem* system);
void boid_system_update(BoidSystem* system, std::shared_ptr<rt::Buffer> instance_data_buffer, float dt);
