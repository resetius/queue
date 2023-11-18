cmake_minimum_required(VERSION 3.10)
project(queue)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

add_executable(queue queue.cpp)
add_executable(client client.cpp)
add_executable(server server.cpp)
add_executable(bench bench.cpp)
