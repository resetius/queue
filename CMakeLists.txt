cmake_minimum_required(VERSION 3.10)
project(queue)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(queue queue.c)
add_executable(client client.c)
add_executable(server server.c)
