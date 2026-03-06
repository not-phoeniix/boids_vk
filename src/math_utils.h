#pragma once

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>

namespace Utils {
    float randf_range(float min, float max);
    glm::vec3 safe_norm(const glm::vec3& vec);
    glm::vec3 safe_norm_len(const glm::vec3& vec, float* out_initial_len);
    glm::vec3 get_rot_look_at(const glm::vec3& source, const glm::vec3& target);
    glm::vec3 get_forward(float pitch, float yaw);
}
