#include <iostream>
#include "render_thing/render_thing.h"
#include <stdexcept>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "input.h"
#include "scene.h"
#include <cstdlib>
#include "thread_pool.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr float FPS_AVG_INTERVAL = 1.0f;

static void run() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "!! boids_vk !!", nullptr, nullptr);

    RenderThing::GraphicsManager graphics(window);
    Scene scene;

    float dt_sum = 0;
    uint32_t frame_counter = 0;

    // seed random with current time
    srand((unsigned int)time(NULL));

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

static void print(uint32_t thred_index) {
}

static void test() {
    ThreadPool pool(4);

    for (uint32_t i = 0; i < 100; i++) {
        std::cout << "adding job " << i << "\n";
        pool.QueueJob([i](uint32_t index) {
            std::cout << "thread: " << index << " ... i: " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    std::cout << "WAIT BEGIN\n";
    pool.Wait();
    std::cout << "pool done waiting :]\n";

    for (uint32_t i = 100; i < 200; i++) {
        std::cout << "adding job " << i << "\n";
        pool.QueueJob([i](uint32_t index) {
            std::cout << "thread: " << index << " ... i: " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    std::cout << "WAIT BEGIN\n";
    pool.Wait();
    std::cout << "pool done waiting :]\n";
}

int main() {
    try {
        // run();
        test();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
