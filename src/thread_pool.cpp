#include "thread_pool.h"
#include <chrono>
#include <iostream>

ThreadPool::ThreadPool(uint32_t thread_count)
  : thread_count(thread_count) {
    Start();
}

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) { }

ThreadPool::~ThreadPool() {
    End();
}

void ThreadPool::Work(uint32_t thread_index) {
    while (running) {
        if (job_queue.empty()) {
            wait_cd.notify_all();
        }

        std::function<void(uint32_t)> func;

        {
            std::unique_lock<std::mutex> lock(mtx);

            queue_cd.wait(lock, [this] {
                return !job_queue.empty() || !running;
            });

            if (!running && job_queue.empty()) {
                return;
            }

            worker_threads_idle[thread_index] = false;

            func = job_queue.front();
            job_queue.pop();
        }

        func(thread_index);

        {
            std::lock_guard<std::mutex> lock(mtx);
            worker_threads_idle[thread_index] = true;
        }
    }
}

bool ThreadPool::IsIdle() {
    for (size_t i = 0; i < worker_threads_idle.size(); i++) {
        if (!worker_threads_idle[i]) return false;
    }

    return true;
}

void ThreadPool::Start() {
    running = true;

    worker_threads_idle.resize(thread_count);
    for (uint32_t i = 0; i < thread_count; i++) {
        worker_threads.emplace_back(&ThreadPool::Work, this, i);
        worker_threads_idle[i] = true;
    }
}

void ThreadPool::End() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }

    queue_cd.notify_all();

    for (auto& thread : worker_threads) {
        thread.join();
    }

    worker_threads.clear();
    worker_threads_idle.clear();
}

void ThreadPool::QueueJob(std::function<void(uint32_t)> func) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        job_queue.push(func);
    }
    queue_cd.notify_one();
}

void ThreadPool::Wait() {
    std::unique_lock<std::mutex> lock(mtx);
    wait_cd.wait(lock, [this] { return job_queue.empty() && IsIdle(); });
}
