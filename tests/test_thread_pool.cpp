#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "logforge/concurrency/thread_pool.hpp"

namespace logforge {
namespace {

TEST(ThreadPoolTest, ExecutesSingleTask) {
    ThreadPool pool(2);
    auto future = pool.submit([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, ExecutesMultipleTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(pool.submit([&counter]() { counter.fetch_add(1); }));
    }

    for (auto& future : futures) {
        future.get();
    }

    EXPECT_EQ(counter.load(), 20);
}

TEST(ThreadPoolTest, WaitIdleBlocksUntilComplete) {
    ThreadPool pool(2);
    std::atomic<bool> done{false};

    pool.submit([&done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        done.store(true);
    });

    pool.wait_idle();
    EXPECT_TRUE(done.load());
}

TEST(ThreadPoolTest, ReportsThreadCount) {
    ThreadPool pool(8);
    EXPECT_EQ(pool.num_threads(), 8);
}

TEST(ThreadPoolTest, DefaultsToAtLeastOneThread) {
    ThreadPool pool(0);
    EXPECT_GE(pool.num_threads(), 1);
}

TEST(ThreadPoolTest, ReturnsComputedValues) {
    ThreadPool pool(2);
    auto future = pool.submit([](int a, int b) { return a + b; }, 3, 4);
    EXPECT_EQ(future.get(), 7);
}

}  // namespace
}  // namespace logforge
