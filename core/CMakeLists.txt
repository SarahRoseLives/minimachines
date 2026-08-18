cmake_minimum_required(VERSION 3.20)

add_library(core STATIC
    src/tiles.cpp
    src/map_data.cpp
    src/map_serializer.cpp
    src/tileset.cpp
    src/car.cpp
    src/collision.cpp
    src/race.cpp
    src/bot.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(core PUBLIC nlohmann_json::nlohmann_json SDL2::SDL2 SDL2_image::SDL2_image)
