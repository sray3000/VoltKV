cmake_minimum_required(VERSION 3.15)
project(VoltKV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apply high-performance optimization flags globally
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# Include header path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(kvstore src/main.cpp src/CmdParser.cpp src/Server.cpp src/KVstore.cpp)
add_executable(kvstore_test tests/KVStoreTest.cpp src/KVStore.cpp)