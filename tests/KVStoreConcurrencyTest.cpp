#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include "KVStore.h"

void testConcurrentSet() {
    KVStore store;

    const int NUM_THREADS = 4;
    const int OPERATIONS_PER_THREAD = 1000;

    std::vector<std::thread> threads;

    for(int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&store, t]() {
            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
                std::string key = "key_" +
                                  std::to_string(t) + "_" +
                                  std::to_string(i);

                std::string value = "value_" +
                                    std::to_string(t) + "_" +
                                    std::to_string(i);

                store.set(key, value);
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    for(int t = 0; t < NUM_THREADS; t++) {
        for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
            std::string key = "key_" +
                              std::to_string(t) + "_" +
                              std::to_string(i);

            std::string expected = "value_" +
                                   std::to_string(t) + "_" +
                                   std::to_string(i);

            auto value = store.get(key);

            assert(value.has_value());
            assert(*value == expected);
        }
    }

    std::cout << "testConcurrentSet passed\n";
}

void testConcurrentRead() {
    KVStore store;

    store.set("counter", "100");

    const int NUM_THREADS = 8;
    const int READS_PER_THREAD = 1000;

    std::vector<std::thread> threads;

    for(int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&store]() {
            for(int i = 0; i < READS_PER_THREAD; i++) {
                auto value = store.get("counter");

                assert(value.has_value());
                assert(*value == "100");

                assert(store.exists("counter"));
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    std::cout << "testConcurrentRead passed\n";
}

void testConcurrentDifferentKeys() {
    KVStore store;

    const int NUM_THREADS = 8;
    const int OPERATIONS_PER_THREAD = 1000;

    std::vector<std::thread> threads;

    for(int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&store, t]() {
            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
                std::string key = "thread_" +
                                  std::to_string(t) + "_" +
                                  std::to_string(i);

                store.set(key, "value");
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    for(int t = 0; t < NUM_THREADS; t++) {
        for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {
            std::string key = "thread_" +
                              std::to_string(t) + "_" +
                              std::to_string(i);

            assert(store.exists(key));

            auto value = store.get(key);

            assert(value.has_value());
            assert(*value == "value");
        }
    }

    std::cout << "testConcurrentDifferentKeys passed\n";
}

int main() {
    testConcurrentSet();
    testConcurrentRead();
    testConcurrentDifferentKeys();

    std::cout << "\nAll concurrency tests passed!\n";

    return 0;
}