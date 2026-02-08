#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <stdint.h>
#include <functional>
#include <queue>
#include <condition_variable>

class ThreadPool {
   private:
    std::vector<std::thread> worker_threads;
    std::vector<bool> worker_threads_idle;
    std::queue<std::function<void(uint32_t)>> job_queue;
    std::mutex mtx;
    std::condition_variable queue_cd;
    std::condition_variable wait_cd;
    uint32_t thread_count;
    bool running;

    void Work(uint32_t thread_index);
    bool IsIdle();

   public:
    ThreadPool(uint32_t thread_count);
    ThreadPool();
    ~ThreadPool();

    void Start();
    void End();

    void QueueJob(std::function<void(uint32_t)> func);
    void Wait();

    uint32_t get_thread_count() const { return thread_count; }
};
