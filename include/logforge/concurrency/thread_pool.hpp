#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace logforge {

/// @brief Fixed-size thread pool for parallel task execution.
///
/// Workers pull tasks from a shared queue. Tasks are type-erased via std::function.
/// The pool shuts down gracefully in the destructor, completing queued work first.
///
/// Thread safety: submit() is safe to call from multiple threads.
class ThreadPool {
   public:
    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief Enqueue a callable and return a future for its result.
    template <typename F, typename... Args>
    auto submit(F&& func, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    /// @brief Block until all queued tasks have completed.
    void wait_idle();

    [[nodiscard]] std::size_t num_threads() const noexcept;

   private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable idle_condition_;
    bool shutdown_{false};
    std::size_t active_tasks_{0};
};

template <typename F, typename... Args>
auto ThreadPool::submit(F&& func, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...));

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard lock(mutex_);
        tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return result;
}

}  // namespace logforge
