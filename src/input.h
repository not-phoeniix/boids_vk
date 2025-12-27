#pragma once

#include <GLFW/glfw3.h>

namespace Input {
    void Update(GLFWwindow* window);

    bool should_exit();
}
