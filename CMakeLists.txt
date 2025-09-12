cmake_minimum_required(VERSION 3.25)
project(EvoArena)

set(CMAKE_CXX_STANDARD 20)
# Chercher SDL2_gfx
find_path(SDL2_GFX_INCLUDE_DIR SDL2/SDL2_gfxPrimitives.h)
find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_GFX_INCLUDE_DIR})

add_executable(EvoArena main.cpp graphics.cpp)

target_link_libraries(EvoArena ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARY})
