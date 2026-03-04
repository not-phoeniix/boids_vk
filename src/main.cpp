#include <iostream>
#include "render_thing/render_thing.h"
#include <stdexcept>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "input.h"
#include "scene.h"
#include <cstdlib>
#include "thread_pool.h"
#include "graphics.h"
#include "program_params.h"
#include "jump_skipfield.h"

static void run() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(
        ProgramParams::WINDOW_WIDTH,
        ProgramParams::WINDOW_HEIGHT,
        "!! boids_vk !!",
        nullptr,
        nullptr
    );

    Graphics::init(window);
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

        Graphics::Manager->Begin();
        scene->Draw();
        Graphics::Manager->EndAndPresent();

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

    vkDeviceWaitIdle(Graphics::Manager->get_device());

    scene.reset();
    Graphics::deinit();

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    // try {
    //     run();
    // } catch (const std::exception& e) {
    //     std::cerr << e.what() << std::endl;
    //     return EXIT_FAILURE;
    // }

    JumpSkipfield<uint32_t> skipfield(20);
    std::cout << "printing empty...\n";
    // skipfield.print_skipfield();
    // skipfield.print_data();
    std::cout << "\n";

    std::cout << "adding...\n";
    for (uint32_t i = 0; i < 10; i++) {
        skipfield.add(i);
    }
    std::cout << "\n";

    std::cout << "printing post add...\n";
    // skipfield.print_skipfield();
    // skipfield.print_data();
    std::cout << "\n";

    std::cout << "iterating...\n";
    skipfield.iterate([](const uint32_t* e, uint32_t a) {
        std::cout << *e << "\n";
    });

    std::cout << "removing some...\n";
    for (uint32_t i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            skipfield.remove_at(i);
        }
    }
    std::cout << "\n";

    std::cout << "printing post remove...\n";
    skipfield.print_skipfield();
    skipfield.print_data();
    std::cout << "\n";

    std::cout << "iterating...\n";
    skipfield.iterate([](const uint32_t* e, uint32_t a) {
        std::cout << *e << "\n";
    });

    return EXIT_SUCCESS;
}
