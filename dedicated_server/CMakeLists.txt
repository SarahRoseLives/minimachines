cmake_minimum_required(VERSION 3.20)

add_executable(dedicated_server
    src/main.cpp
    src/server_game.cpp
)
target_include_directories(dedicated_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dedicated_server PRIVATE core enet::enet nlohmann_json::nlohmann_json httplib)
