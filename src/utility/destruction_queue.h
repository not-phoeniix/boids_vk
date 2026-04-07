#pragma once

#include <deque>
#include <functional>

class DestructionQueue {
   private:
    std::deque<std::function<void()>> queue;

   public:
    DestructionQueue();
    ~DestructionQueue();

    void QueueDelete(std::function<void()> func);
    void Flush();
};
