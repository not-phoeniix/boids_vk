#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "input.h"
#include "scene.h"
#include "graphics/graphics.h"
#include "program_params.h"
#include "utility/check_macros.h"

static bool run() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    BOOL_CHECK(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(
        ProgramParams::WINDOW_WIDTH,
        ProgramParams::WINDOW_HEIGHT,
        "!! boids_vk !!",
        nullptr,
        nullptr
    );

    BOOL_CHECK(Graphics::init(window));
    auto scene = std::make_unique<Scene>();

    float dt_sum = 0;
    uint32_t frame_counter = 0;

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

        scene->Update(dt);

        BOOL_CHECK(Graphics::render_frame([&scene] { scene->Draw(); }));

        frame_counter++;
        dt_sum += dt;

        if (dt_sum >= ProgramParams::FPS_AVG_INTERVAL) {
            float avg_fps = 1.0f / (dt_sum / (float)frame_counter);
            std::cout << "avg fps: " << avg_fps << std::endl;

            dt_sum = 0;
            frame_counter = 0;
        }

        time_prev = time_now;
    }

    vkDeviceWaitIdle(Graphics::get_device());

    scene.reset();
    Graphics::deinit();

    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
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
