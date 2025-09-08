#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                    
                    if(this->stop && this->tasks.empty())
                        return;
                        
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "线程池任务中发生异常: " << e.what() << std::endl;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}
