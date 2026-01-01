#include <iostream>
#include "graphics_manager.h"
#include <stdexcept>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "input.h"
#include "scene.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr float FPS_AVG_INTERVAL = 1.0f;

static void run() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "!! boids_vk !!", nullptr, nullptr);

    GraphicsManager graphics(window);
    Scene scene;

    float dt_sum = 0;
    uint32_t frame_counter = 0;

    scene.Init(graphics);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        Input::Update(window);

        static float time_prev = static_cast<float>(glfwGetTime());
        float time_now = static_cast<float>(glfwGetTime());

        float dt = time_now - time_prev;
        if (dt < 0.0001f) dt = 0.0001f;

        if (Input::get_should_exit()) {
            glfwSetWindowShouldClose(window, true);
        }

        scene.Update(dt);

        graphics.Begin();
        scene.Draw(graphics);
        graphics.EndAndPresent();

        frame_counter++;
        dt_sum += dt;

        if (dt_sum >= FPS_AVG_INTERVAL) {
            float avg_fps = 1.0f / (dt_sum / (float)frame_counter);
            std::cout << "avg fps: " << avg_fps << std::endl;

            dt_sum = 0;
            frame_counter = 0;
        }

        time_prev = time_now;
    }

    vkDeviceWaitIdle(graphics.get_device());

    scene.Deinit();

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
