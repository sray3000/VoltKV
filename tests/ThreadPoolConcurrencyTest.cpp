#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>

#include "ThreadPool.h"

int main() {

    ThreadPool pool(4);

    std::atomic<int> active_tasks{0};
    std::atomic<int> max_active_tasks{0};

    for(int i = 0; i < 8; i++) {

        pool.enqueue([&]() {

            int current = ++active_tasks;

            int previous_max = max_active_tasks.load();

            while(current > previous_max &&
                  !max_active_tasks.compare_exchange_weak(
                      previous_max,
                      current)) {
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );

            --active_tasks;
        });
    }

    // Give workers enough time to execute all tasks.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(300)
    );

    assert(max_active_tasks >= 2);

    return 0;
}