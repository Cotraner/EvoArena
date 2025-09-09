cmake_minimum_required(VERSION 3.25)
project(EvoArena)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(EvoArena main.cpp graphics.cpp)

target_link_libraries(EvoArena ${SDL2_LIBRARIES})

