#include "logforge/concurrency/thread_pool.hpp"

namespace logforge {

ThreadPool::ThreadPool(std::size_t num_threads) {
    if (num_threads == 0) {
        num_threads = 1;
    }

    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        shutdown_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::wait_idle() {
    std::unique_lock lock(mutex_);
    idle_condition_.wait(lock, [this]() { return tasks_.empty() && active_tasks_ == 0; });
}

std::size_t ThreadPool::num_threads() const noexcept {
    return workers_.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this]() { return shutdown_ || !tasks_.empty(); });

            if (shutdown_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
            ++active_tasks_;
        }

        task();

        {
            std::lock_guard lock(mutex_);
            --active_tasks_;
            if (tasks_.empty() && active_tasks_ == 0) {
                idle_condition_.notify_all();
            }
        }
    }
}

}  // namespace logforge
