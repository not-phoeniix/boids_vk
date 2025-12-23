
#include <iostream>
#include "graphics_manager.h"
#include <stdexcept>
#include <cstdlib>
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

void run() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan 2...", nullptr, nullptr);

    GraphicsManager graphics(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // input updates

        static float time_prev = static_cast<float>(glfwGetTime());
        static float time_now = static_cast<float>(glfwGetTime());

        float dt = time_now - time_prev;
        if (dt < 0.0001f) dt = 0.0001f;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    try {
        run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
