cmake_minimum_required(VERSION 3.20)

add_executable(editor WIN32
    src/main.cpp
    src/editor_app.cpp
    src/camera.cpp
    src/tools.cpp
    src/palette.cpp
    src/properties.cpp
)
target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(editor PRIVATE core imgui SDL2::SDL2main SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
target_compile_definitions(editor PRIVATE
    ASSET_PATH="${CMAKE_SOURCE_DIR}/assets"
)
