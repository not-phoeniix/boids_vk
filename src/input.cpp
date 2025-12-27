#include "input.h"

namespace Input {
    namespace {
        bool esc_pressed;
    }
}

void Input::Update(GLFWwindow* window) {
    esc_pressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

bool Input::should_exit() {
    return esc_pressed;
}
