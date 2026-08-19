#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

using Clock = std::chrono::steady_clock;

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

constexpr int OPERATIONS_PER_THREAD = 100000;
constexpr int NUM_THREADS = 1;

// ------------------------------------------------------------
// Mutex-based KV Store
// ------------------------------------------------------------

class MutexKVStore {
private:
    std::unordered_map<int, int> store;
    std::mutex mutex;

public:
    void set(int key, int value) {
        std::lock_guard<std::mutex> lock(mutex);
        store[key] = value;
    }

    int get(int key) {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = store.find(key);

        if(it == store.end())
            return -1;

        return it->second;
    }
};

// ------------------------------------------------------------
// Shared Mutex-based KV Store
// ------------------------------------------------------------

class SharedMutexKVStore {
private:
    std::unordered_map<int, int> store;
    std::shared_mutex mutex;

public:
    void set(int key, int value) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        store[key] = value;
    }

    int get(int key) {
        std::shared_lock<std::shared_mutex> lock(mutex);

        auto it = store.find(key);

        if(it == store.end())
            return -1;

        return it->second;
    }
};

// ------------------------------------------------------------
// Benchmark result
// ------------------------------------------------------------

struct BenchmarkResult {
    double seconds;
    double operations_per_second;
};

// ------------------------------------------------------------
// Mutex benchmark
// ------------------------------------------------------------

BenchmarkResult benchmarkMutex(
    int num_threads,
    double read_ratio
) {
    MutexKVStore store;

    // Pre-populate the store.
    for(int i = 0; i < 10000; i++) {
        store.set(i, i);
    }

    const int operations =
        num_threads * OPERATIONS_PER_THREAD;

    std::vector<std::thread> threads;

    auto start = Clock::now();

    for(int t = 0; t < num_threads; t++) {

        threads.emplace_back([&store, t, read_ratio]() {

            std::mt19937 generator(
                static_cast<unsigned int>(t + 1234)
            );

            std::uniform_int_distribution<int> operation_dist(0, 99);
            std::uniform_int_distribution<int> key_dist(0, 9999);

            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {

                int key = key_dist(generator);

                int operation = operation_dist(generator);

                if(operation < read_ratio * 100) {
                    store.get(key);
                }
                else {
                    store.set(key, key);
                }
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    double seconds =
        std::chrono::duration<double>(end - start).count();

    return {
        seconds,
        operations / seconds
    };
}

// ------------------------------------------------------------
// Shared mutex benchmark
// ------------------------------------------------------------

BenchmarkResult benchmarkSharedMutex(
    int num_threads,
    double read_ratio
) {
    SharedMutexKVStore store;

    // Pre-populate the store.
    for(int i = 0; i < 10000; i++) {
        store.set(i, i);
    }

    const int operations =
        num_threads * OPERATIONS_PER_THREAD;

    std::vector<std::thread> threads;

    auto start = Clock::now();

    for(int t = 0; t < num_threads; t++) {

        threads.emplace_back([&store, t, read_ratio]() {

            std::mt19937 generator(
                static_cast<unsigned int>(t + 1234)
            );

            std::uniform_int_distribution<int> operation_dist(0, 99);
            std::uniform_int_distribution<int> key_dist(0, 9999);

            for(int i = 0; i < OPERATIONS_PER_THREAD; i++) {

                int key = key_dist(generator);

                int operation = operation_dist(generator);

                if(operation < read_ratio * 100) {
                    store.get(key);
                }
                else {
                    store.set(key, key);
                }
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    auto end = Clock::now();

    double seconds =
        std::chrono::duration<double>(end - start).count();

    return {
        seconds,
        operations / seconds
    };
}

// ------------------------------------------------------------
// Print benchmark result
// ------------------------------------------------------------

void printResult(
    const std::string& workload,
    int num_threads,
    BenchmarkResult mutex_result,
    BenchmarkResult shared_result
) {
    double improvement =
        (shared_result.operations_per_second /
         mutex_result.operations_per_second - 1.0) * 100.0;

    std::cout << "\n";
    std::cout << "Workload: " << workload << "\n";
    std::cout << "Threads: " << num_threads << "\n";

    std::cout << "----------------------------------------\n";

    std::cout << "std::mutex\n";
    std::cout << "  Time:       "
              << mutex_result.seconds
              << " s\n";

    std::cout << "  Throughput: "
              << mutex_result.operations_per_second
              << " ops/sec\n";

    std::cout << "\n";

    std::cout << "std::shared_mutex\n";
    std::cout << "  Time:       "
              << shared_result.seconds
              << " s\n";

    std::cout << "  Throughput: "
              << shared_result.operations_per_second
              << " ops/sec\n";

    std::cout << "\n";

    std::cout << "shared_mutex vs mutex: "
              << improvement
              << "%\n";
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

int main() {

    std::cout << "========================================\n";
    std::cout << " VoltKV Lock Performance Benchmark\n";
    std::cout << "========================================\n";

    const std::vector<int> thread_counts = {
        1, 2, 4, 8
    };

    struct Workload {
        std::string name;
        double read_ratio;
    };

    const std::vector<Workload> workloads = {
        {"Read-heavy (90% GET / 10% SET)", 0.90},
        {"Balanced (50% GET / 50% SET)",    0.50},
        {"Write-heavy (10% GET / 90% SET)", 0.10}
    };

    for(const auto& workload : workloads) {

        for(int num_threads : thread_counts) {

            BenchmarkResult mutex_result =
                benchmarkMutex(
                    num_threads,
                    workload.read_ratio
                );

            BenchmarkResult shared_result =
                benchmarkSharedMutex(
                    num_threads,
                    workload.read_ratio
                );

            printResult(
                workload.name,
                num_threads,
                mutex_result,
                shared_result
            );
        }
    }

    std::cout << "\n========================================\n";
    std::cout << " Benchmark complete\n";
    std::cout << "========================================\n";

    return 0;
}