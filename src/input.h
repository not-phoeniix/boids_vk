#pragma once

#include "render_thing/glm_settings.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Input {
    void Update(GLFWwindow* window);

    bool get_should_exit();
    bool get_lmb_down();
    bool get_is_sprinting();
    bool get_light_refresh();
    glm::vec3 get_move_axis();
    glm::vec2 get_mouse_delta();
}
