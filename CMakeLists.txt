cmake_minimum_required(VERSION 3.14)
project(chip8)

set(CMAKE_CXX_STANDARD 11)

# SDL Only
# add_subdirectory(3rdParty/SDL2-2.0.20 EXCLUDE_FROM_ALL)

add_executable(
    chip8
    src/chip8.cpp
    src/main.cpp
    src/platform.cpp
)

target_compile_options(chip8 PRIVATE -Wall)

# Link SDL2 only
target_link_libraries(chip8 PRIVATE SDL2)
