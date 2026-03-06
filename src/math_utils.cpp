#include "math_utils.h"

float Utils::randf_range(float min, float max) {
    static uint32_t state = static_cast<uint32_t>(time(NULL));

    //   https://en.wikipedia.org/wiki/Xorshift
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    state = x;

    return min + ((max - min) * ((float)x / (float)UINT32_MAX));
}

// a hopefully more cache-friendly safe normalization method?
glm::vec3 Utils::safe_norm(const glm::vec3& vec) {
    glm::vec3 ret = vec;
    float len = glm::length(vec);
    bool safe = len > FLT_EPSILON;

    // ensures we're never dividing by zero
    ret *= 1.0f / (len + static_cast<float>(!safe));

    return ret;
}

// a hopefully more cache-friendly safe normalization method?
glm::vec3 Utils::safe_norm_len(const glm::vec3& vec, float* out_initial_len) {
    glm::vec3 ret = vec;
    float len = glm::length(vec);
    bool safe = len > FLT_EPSILON;

    // ensures we're never dividing by zero
    ret *= 1.0f / (len + static_cast<float>(!safe));

    *out_initial_len = len;
    return ret;
}

glm::vec3 Utils::get_rot_look_at(const glm::vec3& source, const glm::vec3& target) {
    glm::vec3 delta = safe_norm(target - source);
    float yaw = atan2f(delta.x, delta.z);
    float pitch = asinf(-delta.y);
    return glm::vec3(pitch, yaw, 0);
}

glm::vec3 Utils::get_forward(float pitch, float yaw) {
    float x = std::cosf(pitch) * std::sinf(yaw);
    float y = std::sinf(pitch);
    float z = std::cosf(pitch) * std::cosf(yaw);
    return glm::vec3(x, y, z);
}
