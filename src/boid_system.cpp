#include "boid_system.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <cmath>
#include <algorithm>
#include "data_structs.h"
#include "program_params.h"
#include "math_utils.h"

static std::mutex mtx;

#pragma region // behaviors

static glm::vec3 seek(const glm::vec3& target_pos, BoidSystem* system, uint32_t boid) {
    glm::vec3 dir = Utils::safe_norm(target_pos - system->boid_positions[boid]);
    glm::vec3 desired_velocity = dir * system->boid_max_speeds[boid];
    return desired_velocity;
}

static glm::vec3 flee(const glm::vec3& target_pos, BoidSystem* system, uint32_t boid) {
    glm::vec3 dir = Utils::safe_norm(system->boid_positions[boid] - target_pos);
    glm::vec3 desired_velocity = dir * system->boid_max_speeds[boid];
    return desired_velocity;
}

static glm::vec3 wander(BoidSystem* system, uint32_t boid) {
    glm::vec3 angle_offset = {
        Utils::randf_range(-M_PI / 15.0f, M_PI / 15.0f),
        Utils::randf_range(-M_PI / 15.0f, M_PI / 15.0f),
        Utils::randf_range(-M_PI / 15.0f, M_PI / 15.0f)
    };
    glm::vec3 wander_angles = system->boid_wander_angles[boid];
    wander_angles += angle_offset;
    system->boid_wander_angles[boid] = wander_angles;

    glm::vec3 forward = Utils::get_forward(wander_angles.x, wander_angles.y);

    glm::vec3 future_pos = system->boid_positions[boid] +
                           (system->boid_velocities[boid] *
                            ProgramParams::BOID_WANDER_TIME);
    glm::vec3 target_pos = future_pos + (forward * ProgramParams::BOID_WANDER_RADIUS);

    return seek(target_pos, system, boid);
}

#pragma endregion

static uint32_t get_chunk_index(BoidSystem* system, uint32_t boid) {
    // don't need separate axes since we're always a big cube
    float chunk_width = system->bounds_size / (float)ProgramParams::CHUNKS_PER_AXIS;
    float min_coord = system->bounds_size / 2.0f;

    glm::vec3 current_pos = system->boid_positions[boid];

    // use truncation and some position math to get indices
    glm::u32vec3 indices = {
        (uint32_t)((current_pos.x + min_coord) / chunk_width),
        (uint32_t)((current_pos.y + min_coord) / chunk_width),
        (uint32_t)((current_pos.z + min_coord) / chunk_width)
    };

    indices = glm::clamp(
        indices,
        glm::u32vec3(0),
        glm::u32vec3(ProgramParams::CHUNKS_PER_AXIS - 1)
    );

    return indices.z * ProgramParams::CHUNKS_PER_AXIS * ProgramParams::CHUNKS_PER_AXIS +
           indices.y * ProgramParams::CHUNKS_PER_AXIS +
           indices.x;
}

static void init_boid(BoidSystem* system, uint32_t boid) {
    system->boid_positions[boid] = {
        Utils::randf_range(-system->bounds_size, system->bounds_size),
        Utils::randf_range(-system->bounds_size, system->bounds_size),
        Utils::randf_range(-system->bounds_size, system->bounds_size)
    };

    system->boid_velocities[boid] = {
        Utils::randf_range(-10.0f, 10.0f),
        Utils::randf_range(-10.0f, 10.0f),
        Utils::randf_range(-10.0f, 10.0f)
    };

    system->boid_wander_angles[boid] = {0.0f, 0.0f, 0.0f};
    system->boid_contained_chunk_index[boid] = get_chunk_index(system, boid);
    system->boid_max_speeds[boid] = Utils::randf_range(
        ProgramParams::BOID_MAX_SPEED_MIN,
        ProgramParams::BOID_MAX_SPEED_MAX
    );
}

void boid_system_populate(BoidSystem* system, uint32_t count, float bounds_size) {
    system->boid_positions.resize(count);
    system->boid_velocities.resize(count);
    system->boid_wander_angles.resize(count);
    system->boid_contained_chunk_index.resize(count);
    system->boid_max_speeds.resize(count);
    system->boid_count = count;
    system->chunks.resize(
        ProgramParams::CHUNKS_PER_AXIS *
        ProgramParams::CHUNKS_PER_AXIS *
        ProgramParams::CHUNKS_PER_AXIS
    );
    system->bounds_size = bounds_size;
    system->thread_pool = std::make_shared<ThreadPool>();

    for (uint32_t i = 0; i < count; i++) {
        init_boid(system, i);
    }
}

void boid_system_destroy(BoidSystem* system) {
    system->boid_positions.clear();
    system->boid_velocities.clear();
    system->boid_wander_angles.clear();
    system->boid_contained_chunk_index.clear();
    system->boid_max_speeds.clear();
    system->chunks.clear();
    system->thread_pool.reset();

    system->bounds_size = 0.0f;
    system->thread_pool = nullptr;
}

static void update_boid_container(uint32_t b, BoidSystem* system) {
    uint32_t current_index = system->boid_contained_chunk_index[b];
    uint32_t desired_index = get_chunk_index(system, b);

    {
        std::lock_guard<std::mutex> lock(mtx);
        system->chunks[current_index].erase(b);
        system->chunks[desired_index].insert(b);
    }

    system->boid_contained_chunk_index[b] = desired_index;
}

static void update_boid(
    uint32_t b,
    BoidSystem* system,
    Buffer* instance_data_buffer,
    float dt
) {
    glm::vec3 avg_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 avg_direction = {0.0f, 0.0f, 0.0f};
    uint32_t num_adjacent = 0;

    glm::vec3 total_steer = {0.0f, 0.0f, 0.0f};

    for (const auto& b2 : system->chunks[system->boid_contained_chunk_index[b]]) {
        glm::vec3 diff = system->boid_positions[b] - system->boid_positions[b2];
        float d_sqr = glm::length2(diff);

        bool in_range = d_sqr > FLT_EPSILON && d_sqr <= ProgramParams::BOID_ADJACENT_SEARCH_RADIUS * ProgramParams::BOID_ADJACENT_SEARCH_RADIUS;
        if (in_range) {
            // ~~~ separation, flee from neighbors in range ~~~

            total_steer += flee(system->boid_positions[b2], system, b) * ProgramParams::BOID_SEPARATE_STRENGTH;

            // ~~~ accumulate averages ~~~

            avg_position += system->boid_positions[b2];
            avg_direction += Utils::safe_norm(system->boid_velocities[b2]);
            num_adjacent++;
        }
    }

    {
        // using funny boolean multiplication here to avoid conditionals for caching

        // ensure we never divide by zero
        float denominator = num_adjacent + (1.0f * static_cast<float>(num_adjacent == 0));

        // ~~~ cohesion, seek avg ~~~
        avg_position *= 1.0f / denominator;
        total_steer += (seek(avg_position, system, b) * ProgramParams::BOID_COHESION_STRENGTH) *
                       static_cast<float>(num_adjacent != 0);

        // ~~~ alignment, move towards desired direction ~~~
        avg_direction *= (1.0f / denominator) * system->boid_max_speeds[b];
        glm::vec3 desired_dir = avg_direction - system->boid_velocities[b];
        total_steer += (desired_dir * ProgramParams::BOID_ALIGNMENT_STRENGTH) *
                       static_cast<float>(num_adjacent != 0);
    }

    // ~~~ wander! ~~~
    total_steer += wander(system, b) * ProgramParams::BOID_WANDER_STRENGTH;

    // ~~~ seek center ~~~
    //   so we don't run away forever (only when we're past the threshold)
    float d_sqr = glm::length2(system->boid_positions[b]);
    total_steer += seek(glm::vec3(0.0f), system, b) *
                   static_cast<float>(d_sqr >= (system->bounds_size * system->bounds_size)) *
                   ProgramParams::BOID_BOUND_LIMIT_STRENGTH;

    // ~~~ friction !! ~~~

    float vel_len = 0.0f;
    glm::vec3 dir = Utils::safe_norm_len(system->boid_velocities[b], &vel_len);

    total_steer += dir * (ProgramParams::BOID_FRICTION_COEFF * -1.0f);

    // ~~~ cap velocity ~~~

    {
        bool too_fast = vel_len > system->boid_max_speeds[b];
        float denominator = vel_len + (1.0f * static_cast<float>(vel_len <= FLT_EPSILON));
        float scalar = (1.0f / denominator) * system->boid_max_speeds[b];

        system->boid_velocities[b] *= scalar * static_cast<float>(too_fast) +
                                      (1.0f * static_cast<float>(!too_fast));
    }

    // ~~~ update positions using euler method! ~~~

    system->boid_velocities[b] += total_steer * dt;
    system->boid_positions[b] += system->boid_velocities[b] * dt;

    // ~~~ update boid transform matrices ~~~

    glm::vec3 pos = system->boid_positions[b];
    glm::vec3 rot = Utils::get_rot_look_at(pos, pos + system->boid_velocities[b]);
    glm::mat4 world = glm::translate(glm::mat4(1.0f), pos);
    // we don't need a Z rot because Utils::get_rot_look_at
    //   is only pitch/yaw, no roll
    world = glm::rotate(world, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));

    buffer_copy_host(
        instance_data_buffer,
        &world,
        sizeof(world),
        sizeof(glm::mat4x4) * b
    );
}

void boid_system_update(BoidSystem* system, Buffer* instance_data_buffer, float dt) {
    // UPDATE CONTAINERS IN BATCHES
    {
        uint32_t b_start = 0;
        uint32_t remaining = system->boid_count;

        for (uint32_t i = 0; i < system->thread_pool->get_thread_count(); i++) {
            uint32_t count = system->boid_count / system->thread_pool->get_thread_count();
            if (count > remaining) count = remaining;

            system->thread_pool->QueueJob(
                [b_start, count, system](uint32_t thread_index) {
                    for (uint32_t b = b_start; b < b_start + count; b++) {
                        update_boid_container(b, system);
                    }
                }
            );

            b_start += count;
            remaining -= count;
        }

        system->thread_pool->Wait();
    }

    // UPDATE BOID POSITIONS IN BATCHES
    {
        uint32_t b_start = 0;
        uint32_t remaining = system->boid_count;

        for (uint32_t i = 0; i < system->thread_pool->get_thread_count(); i++) {
            uint32_t count = system->boid_count / system->thread_pool->get_thread_count();
            if (count > remaining) count = remaining;

            system->thread_pool->QueueJob(
                [b_start, count, system, instance_data_buffer, dt](uint32_t thread_index) {
                    for (uint32_t b = b_start; b < b_start + count; b++) {
                        update_boid(b, system, instance_data_buffer, dt);
                    }
                }
            );

            b_start += count;
            remaining -= count;
        }

        system->thread_pool->Wait();
    }
}
