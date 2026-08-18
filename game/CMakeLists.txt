cmake_minimum_required(VERSION 3.20)

add_executable(game WIN32
    src/main.cpp
    src/game_app.cpp
    src/camera.cpp
    src/input.cpp
)
target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(game PRIVATE core imgui SDL2::SDL2main SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
