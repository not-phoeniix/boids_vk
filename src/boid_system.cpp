#include "boid_system.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

constexpr uint32_t CHUNKS_PER_AXIS = 8;

constexpr float ADJACENT_SEARCH_RADIUS = 10.0f;
constexpr float SEPARATE_STRENGTH = 0.4f;
constexpr float COHESION_STRENGTH = 3.0f;
constexpr float ALIGNMENT_STRENGTH = 2.0f;

constexpr float WANDER_STRENGTH = 1.0f;
constexpr float WANDER_TIME = 0.4f;
constexpr float WANDER_RADIUS = 50.0f;

constexpr float LIMIT_DISTANCE = 100.0f;
constexpr float LIMIT_STRENGTH = 4.0f;

constexpr float FRICTION_COEFF = 2.0f;

constexpr float MAX_SPEED_MIN = 10.0f;
constexpr float MAX_SPEED_MAX = 50.0f;

static float randf_range(float min, float max) {
    return min + ((max - min) * ((rand() / (float)RAND_MAX)));
}

#pragma region // wandering behaviors

static glm::vec3 seek(const glm::vec3& target_pos, BoidSystem* system, uint32_t boid) {
    glm::vec3 dir = target_pos - system->boid_positions[boid];

    // a hopefully more cache-friendly safe normalization method?
    float len = glm::length(dir);
    dir *= (len > FLT_EPSILON) ? (1.0f / len) : 1.0f;

    glm::vec3 desired_velocity = dir * system->boid_max_speeds[boid];

    return desired_velocity;
}

static glm::vec3 flee(const glm::vec3& target_pos, BoidSystem* system, uint32_t boid) {
    glm::vec3 dir = system->boid_positions[boid] - target_pos;

    // a hopefully more cache-friendly safe normalization method?
    float len = glm::length(dir);
    dir *= (len > FLT_EPSILON) ? (1.0f / len) : 1.0f;

    glm::vec3 desired_velocity = dir * system->boid_max_speeds[boid];

    return desired_velocity;
}

static glm::vec3 wander(BoidSystem* system, uint32_t boid) {
    glm::vec3 angle_offset = {
        randf_range(-M_PI / 15.0f, M_PI / 15.0f),
        randf_range(-M_PI / 15.0f, M_PI / 15.0f),
        randf_range(-M_PI / 15.0f, M_PI / 15.0f)
    };
    system->boid_wander_angles[boid] += angle_offset;

    //! maybe check back here for optimizations! calculating a rotation
    //!  matrix for every boid every frame can't be efficient <//3
    glm::mat4 rot_mat = glm::rotate(
        glm::mat4(1.0f),
        system->boid_wander_angles[boid].z,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    rot_mat = glm::rotate(
        rot_mat,
        system->boid_wander_angles[boid].y,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    rot_mat = glm::rotate(
        rot_mat,
        system->boid_wander_angles[boid].x,
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    glm::vec4 forward = rot_mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    glm::vec3 future_pos = system->boid_positions[boid] + (system->boid_velocities[boid] * WANDER_TIME);
    glm::vec3 target_pos = future_pos + (glm::vec3(forward) * WANDER_RADIUS);

    return seek(target_pos, system, boid);
}

#pragma endregion

static uint32_t get_chunk_index(BoidSystem* system, uint32_t boid) {
    // don't need separate axes since we're always a big cube
    float chunk_width = system->bounds_size / (float)CHUNKS_PER_AXIS;
    float min_coord = system->bounds_size / 2.0f;

    // use truncation and some position math to get indices
    glm::u32vec3 indices = {
        (uint32_t)((system->boid_positions[boid].x + min_coord) / chunk_width),
        (uint32_t)((system->boid_positions[boid].y + min_coord) / chunk_width),
        (uint32_t)((system->boid_positions[boid].z + min_coord) / chunk_width)
    };

    indices = glm::clamp(
        indices,
        glm::u32vec3(0),
        glm::u32vec3(CHUNKS_PER_AXIS - 1)
    );

    return indices.z * CHUNKS_PER_AXIS * CHUNKS_PER_AXIS + indices.y * CHUNKS_PER_AXIS + indices.x;
}

static void init_boid(BoidSystem* system, uint32_t boid) {
    system->boid_positions[boid] = {
        randf_range(-system->bounds_size / 2.0f, system->bounds_size / 2.0f),
        randf_range(-system->bounds_size / 2.0f, system->bounds_size / 2.0f),
        randf_range(-system->bounds_size / 2.0f, system->bounds_size / 2.0f)
    };

    system->boid_velocities[boid] = {
        randf_range(-10.0f, 10.0f),
        randf_range(-10.0f, 10.0f),
        randf_range(-10.0f, 10.0f)
    };

    system->boid_wander_angles[boid] = {0.0f, 0.0f, 0.0f};
    system->boid_contained_chunk_index[boid] = get_chunk_index(system, boid);
    system->boid_max_speeds[boid] = randf_range(MAX_SPEED_MIN, MAX_SPEED_MAX);
}

void boid_system_populate(BoidSystem* system, uint32_t count, float bounds_size) {
    system->boid_positions = new glm::vec3[count];
    system->boid_velocities = new glm::vec3[count];
    system->boid_wander_angles = new glm::vec3[count];
    system->boid_contained_chunk_index = new uint32_t[count];
    system->boid_max_speeds = new float[count];
    system->boid_count = count;
    system->chunks = new BoidChunk[CHUNKS_PER_AXIS * CHUNKS_PER_AXIS * CHUNKS_PER_AXIS];
    system->bounds_size = bounds_size;

    for (uint32_t i = 0; i < count; i++) {
        init_boid(system, i);
    }
}

void boid_system_destroy(BoidSystem* system) {
    delete[] system->boid_positions;
    delete[] system->boid_velocities;
    delete[] system->boid_wander_angles;
    delete[] system->boid_contained_chunk_index;
    delete[] system->boid_max_speeds;
    delete[] system->chunks;

    system->boid_positions = nullptr;
    system->boid_velocities = nullptr;
    system->boid_wander_angles = nullptr;
    system->boid_contained_chunk_index = nullptr;
    system->boid_max_speeds = nullptr;
    system->boid_count = 0;
    system->chunks = nullptr;
    system->bounds_size = 0.0f;
}

void boid_system_update(BoidSystem* system, float dt) {
    for (uint32_t b = 0; b < system->boid_count; b++) {
        glm::vec3 avg_position = {0.0f, 0.0f, 0.0f};
        glm::vec3 avg_direction = {0.0f, 0.0f, 0.0f};
        uint32_t num_adjacent = 0;

        glm::vec3 total_steer = {0.0f, 0.0f, 0.0f};

        // always erase prev chunk and re-place for cache efficiency
        system->chunks[system->boid_contained_chunk_index[b]].erase(b);
        uint32_t chunk_index = get_chunk_index(system, b);
        system->boid_contained_chunk_index[b] = chunk_index;
        system->chunks[chunk_index].insert(b);

        for (const auto& b2 : system->chunks[chunk_index]) {
            glm::vec3 diff = system->boid_positions[b] - system->boid_positions[b2];
            float d_sqr = glm::length2(diff);

            // ensure we're in range and also not at the same position
            if (d_sqr > FLT_EPSILON && d_sqr <= ADJACENT_SEARCH_RADIUS * ADJACENT_SEARCH_RADIUS) {
                // ~~~ separation, flee from neighbors in range ~~~
                total_steer += flee(system->boid_positions[b2], system, b) * SEPARATE_STRENGTH;

                // ~~~ accumulate average positions and directions ~~~

                avg_position += system->boid_positions[b2];

                glm::vec3 dir = system->boid_velocities[b2];
                float len = glm::length(dir);
                dir *= (len > FLT_EPSILON) ? (1.0f / len) : 1.0f;
                avg_direction += dir;

                num_adjacent++;
            }
        }

        if (num_adjacent > 0) {
            // ~~~ cohesion, seek avg ~~~
            avg_position *= 1.0f / num_adjacent;
            total_steer += seek(avg_position, system, b) * COHESION_STRENGTH;

            // ~~~ alignment, move towards desired direction ~~~
            avg_direction *= (1.0f / num_adjacent) * system->boid_max_speeds[b];
            glm::vec3 desired_dir = avg_direction - system->boid_velocities[b];
            total_steer += desired_dir * ALIGNMENT_STRENGTH;
        }

        // ~~~ wander! ~~~
        total_steer += wander(system, b) * WANDER_STRENGTH;

        // ~~~ seek center ~~~
        //   so we don't run away forever (only when we're past the threshold)
        float d_sqr = glm::length2(system->boid_positions[b]);
        total_steer += seek(glm::vec3(0.0f), system, b) *
                       static_cast<float>(d_sqr >= LIMIT_DISTANCE * LIMIT_DISTANCE) *
                       LIMIT_STRENGTH;

        // ~~~ friction !! ~~~

        glm::vec3 dir = system->boid_velocities[b];
        float vel_len = glm::length(dir);

        // a hopefully more cache-friendly safe normalization method?
        dir *= (vel_len > FLT_EPSILON) ? (1.0f / vel_len) : 1.0f;

        total_steer += dir * (FRICTION_COEFF * -1.0f);

        // ~~~ cap velocity ~~~
        if (vel_len > system->boid_max_speeds[b]) {
            system->boid_velocities[b] = dir * system->boid_max_speeds[b];
        }

        // ~~~ update positions using euler method! ~~~

        system->boid_velocities[b] += total_steer * dt;
        system->boid_positions[b] += system->boid_velocities[b] * dt;
    }
}
