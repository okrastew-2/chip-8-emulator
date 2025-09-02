cmake_minimum_required(VERSION 3.28)
project(CHIP8 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(Chip8Emu src/main.cpp src/chip8.cpp)

#set_target_properties(Chip8Emu PROPERTIES WIN32_EXECUTABLE TRUE)
target_include_directories(Chip8Emu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_compile_features(Chip8Emu PRIVATE cxx_std_17)
target_link_libraries(Chip8Emu PRIVATE SFML::Graphics SFML::Audio)
