cmake_minimum_required(VERSION 3.25)
project(EvoArena)

set(CMAKE_CXX_STANDARD 20)

# --- CORRECTION DE LA RECHERCHE DE SDL2 (Remplacer find_package(SDL2...)) ---
# Utiliser PKG-CONFIG pour trouver les chemins et les drapeaux (plus fiable sous WSL)
# Note : Vous devez avoir le package 'pkg-config' installé sur votre distribution Debian.
find_package(PkgConfig REQUIRED)

# Trouver la librairie SDL2 Core
pkg_check_modules(SDL2 REQUIRED sdl2)

# Trouver les librairies extensions
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)


# Gestion manuelle de SDL2_gfx
find_path(SDL2_GFX_INCLUDE_DIR SDL2/SDL2_gfxPrimitives.h)
find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx)


# --- INCLUDES (Utiliser les chemins trouvés par pkg-config) ---
include_directories(
        ${SDL2_INCLUDE_DIRS} # Utilisation de la variable générée par pkg-config
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

file(GLOB_RECURSE sourceCode src/*.cpp src/*.h)

add_executable(EvoArena ${sourceCode})

# --- LIAISON (Utiliser les variables générées par pkg-config) ---
target_link_libraries(EvoArena
        ${SDL2_LIBRARIES} # Utilisation de la variable générée par pkg-config
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_GFX_LIBRARY}
)