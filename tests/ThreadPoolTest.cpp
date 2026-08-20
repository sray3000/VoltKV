#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>

#include "ThreadPool.h"

int main() {

    std::atomic<int> counter{0};

    {
        ThreadPool pool(4);

        for(int i = 0; i < 20; i++) {
            pool.enqueue([&counter]() {
                counter++;
            });
        }

        // Give workers time to execute the tasks.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }

    assert(counter == 20);

    return 0;
}