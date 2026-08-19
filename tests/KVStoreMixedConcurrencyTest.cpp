#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include "KVStore.h"

void testMixedReadWrite() {
    KVStore store;

    const int NUM_WRITERS = 4;
    const int NUM_READERS = 4;
    const int OPERATIONS_PER_THREAD = 2000;

    std::vector<std::thread> threads;

    // Writers
    for(int t = 0; t < NUM_WRITERS; t++) {
        threads.emplace_back([&store, t]() {
            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
                std::string key =
                    "key_" +
                    std::to_string(t) + "_" +
                    std::to_string(i);

                std::string value =
                    "value_" +
                    std::to_string(t) + "_" +
                    std::to_string(i);

                store.set(key, value);
            }
        });
    }

    // Readers
    for(int t = 0; t < NUM_READERS; t++) {
        threads.emplace_back([&store, t]() {
            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
                std::string key =
                    "key_" +
                    std::to_string(t % NUM_WRITERS) + "_" +
                    std::to_string(i);

                store.get(key);
                store.exists(key);
            }
        });
    }

    // Wait for all threads
    for(auto& thread : threads) {
        thread.join();
    }

    // Verify every writer's data
    for(int t = 0; t < NUM_WRITERS; t++) {
        for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
            std::string key =
                "key_" +
                std::to_string(t) + "_" +
                std::to_string(i);

            std::string expected =
                "value_" +
                std::to_string(t) + "_" +
                std::to_string(i);

            auto value = store.get(key);

            assert(value.has_value());
            assert(*value == expected);
        }
    }

    std::cout << "testMixedReadWrite passed\n";
}

void testConcurrentDelete() {
    KVStore store;

    const int NUM_THREADS = 4;
    const int KEYS_PER_THREAD = 1000;

    // Populate the store first.
    for(int t = 0; t < NUM_THREADS; t++) {
        for(int i = 0; i < KEYS_PER_THREAD; i++) {
            std::string key =
                "key_" +
                std::to_string(t) + "_" +
                std::to_string(i);

            store.set(key, "value");
        }
    }

    std::vector<std::thread> threads;

    // Each thread deletes its own set of keys.
    for(int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&store, t]() {
            for(int i = 0; i < KEYS_PER_THREAD; i++) {
                std::string key =
                    "key_" +
                    std::to_string(t) + "_" +
                    std::to_string(i);

                store.erase(key);
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    // Everything should now be gone.
    for(int t = 0; t < NUM_THREADS; t++) {
        for(int i = 0; i < KEYS_PER_THREAD; i++) {
            std::string key =
                "key_" +
                std::to_string(t) + "_" +
                std::to_string(i);

            assert(!store.exists(key));
            assert(!store.get(key).has_value());
        }
    }

    std::cout << "testConcurrentDelete passed\n";
}

int main() {
    testMixedReadWrite();
    testConcurrentDelete();

    std::cout << "\nAll mixed concurrency tests passed!\n";

    return 0;
}