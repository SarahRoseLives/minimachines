cmake_minimum_required(VERSION 3.22)
project(minimachines LANGUAGES C CXX)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SDL2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL2)

# SDL2_image
FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-2.8.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL2_image)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

# ENet (manual build - skip its CMakeLists to avoid breaking Android toolchain)
FetchContent_Declare(
    enet_src
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
    GIT_SHALLOW    TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
FetchContent_MakeAvailable(enet_src)

add_library(mm_enet STATIC
    ${enet_src_SOURCE_DIR}/callbacks.c
    ${enet_src_SOURCE_DIR}/compress.c
    ${enet_src_SOURCE_DIR}/host.c
    ${enet_src_SOURCE_DIR}/list.c
    ${enet_src_SOURCE_DIR}/packet.c
    ${enet_src_SOURCE_DIR}/peer.c
    ${enet_src_SOURCE_DIR}/protocol.c
    ${enet_src_SOURCE_DIR}/unix.c
)
target_include_directories(mm_enet PUBLIC ${enet_src_SOURCE_DIR}/include)
target_compile_definitions(mm_enet PRIVATE HAS_SOCKLEN_T)
target_link_libraries(mm_enet PUBLIC android log)

# ImGui (docking)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL2::SDL2)

# Core library
add_library(core STATIC
    ${ROOT_DIR}/core/src/tiles.cpp
    ${ROOT_DIR}/core/src/map_data.cpp
    ${ROOT_DIR}/core/src/map_serializer.cpp
    ${ROOT_DIR}/core/src/tileset.cpp
    ${ROOT_DIR}/core/src/car.cpp
    ${ROOT_DIR}/core/src/collision.cpp
    ${ROOT_DIR}/core/src/race.cpp
    ${ROOT_DIR}/core/src/bot.cpp
)
target_include_directories(core PUBLIC ${ROOT_DIR}/core/include)
target_link_libraries(core PUBLIC nlohmann_json::nlohmann_json SDL2::SDL2 SDL2_image::SDL2_image)

# Game shared library (loaded by SDL Activity - must be named 'main')
add_library(main SHARED
    ${ROOT_DIR}/game/src/main.cpp
    ${ROOT_DIR}/game/src/game_app.cpp
    ${ROOT_DIR}/game/src/camera.cpp
    ${ROOT_DIR}/game/src/input.cpp
    ${ROOT_DIR}/game/src/network_client.cpp
)
target_include_directories(main PRIVATE ${ROOT_DIR}/game/src)
target_link_libraries(main PRIVATE core imgui SDL2::SDL2 SDL2_image::SDL2_image mm_enet httplib android log)
