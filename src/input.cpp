#include "input.h"

namespace Input {
    static bool esc_pressed;
    static bool lmb_down;
    static bool sprinting;
    static bool light_refresh = false;
    static bool light_refresh_prev = false;
    static glm::vec3 move_axis;
    static glm::vec2 mouse_pos_prev;
    static glm::vec2 mouse_pos;
}

void Input::Update(GLFWwindow* window) {
    esc_pressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    lmb_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    light_refresh_prev = light_refresh;
    light_refresh = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

    move_axis.x = 0;
    move_axis.y = 0;
    move_axis.z = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        move_axis.z += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        move_axis.z -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        move_axis.x += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        move_axis.x -= 1;
    }
    if (
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS
    ) {
        move_axis.y += 1;
    }
    if (
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS
    ) {
        move_axis.y -= 1;
    }

    mouse_pos_prev = mouse_pos;

    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    mouse_pos.x = static_cast<float>(x);
    mouse_pos.y = static_cast<float>(y);
}

bool Input::get_should_exit() {
    return esc_pressed;
}

bool Input::get_lmb_down() {
    return lmb_down;
}

bool Input::get_is_sprinting() {
    return sprinting;
}

bool Input::get_light_refresh() {
    return light_refresh && !light_refresh_prev;
}

glm::vec3 Input::get_move_axis() {
    return move_axis;
}

glm::vec2 Input::get_mouse_delta() {
    return mouse_pos - mouse_pos_prev;
}
