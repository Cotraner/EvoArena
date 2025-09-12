cmake_minimum_required(VERSION 3.25)
project(EvoArena)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

find_path(SDL2_GFX_INCLUDE_DIR SDL2/SDL2_gfxPrimitives.h)
find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx)



include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

file(GLOB_RECURSE sourceCode src/*.cpp src/*.h)

add_executable(EvoArena ${sourceCode})

target_link_libraries(EvoArena ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARY} SDL2_image)
