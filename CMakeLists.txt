cmake_minimum_required(VERSION 3.15)
project(VoltKV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# High-performance optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# Include header paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Main server
add_executable(kvstore
    src/main.cpp
    src/CmdParser.cpp
    src/Server.cpp
    src/KVstore.cpp
    src/ThreadPool.cpp
)

# Unit tests
add_executable(kvstore_test
    tests/KVStoreTest.cpp
    src/KVstore.cpp
)
add_executable(
    kvstore_ttl_test
    tests/KVStoreTTLTest.cpp
    src/KVStore.cpp
)
add_executable(
    kvstore_lru_test
    tests/KVStoreLRUTest.cpp
    src/KVStore.cpp
)

# Concurrency tests
add_executable(kvstore_conc_test
    tests/KVStoreConcurrencyTest.cpp
    src/KVstore.cpp
)
add_executable(kvstore_mixed_conc_test
    tests/KVStoreMixedConcurrencyTest.cpp
    src/KVStore.cpp
)

# ThreadPool tests
add_executable(threadpool_test
    tests/ThreadPoolTest.cpp
    src/ThreadPool.cpp
)
add_executable(threadpool_concurrency_test
    tests/ThreadPoolConcurrencyTest.cpp
    src/ThreadPool.cpp
)

# Benchmarks
add_executable(kvstore_lock_benchmark
    benchmarks/KVStoreLockBenchmark.cpp
)
add_executable(
    kvstore_server_benchmark
    benchmarks/ServerBenchmark.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(kvstore_conc_test
    Threads::Threads
)
target_link_libraries(kvstore_mixed_conc_test
    Threads::Threads
)
target_link_libraries(kvstore_lock_benchmark
    Threads::Threads
)
target_link_libraries(threadpool_test
    Threads::Threads
)
target_link_libraries(threadpool_concurrency_test
    Threads::Threads
)