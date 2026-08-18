cmake_minimum_required(VERSION 3.20)
project(minimachines VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(cmake/deps.cmake)

add_subdirectory(core)
add_subdirectory(editor)
add_subdirectory(game)
add_subdirectory(dedicated_server)
add_subdirectory(master_server)
