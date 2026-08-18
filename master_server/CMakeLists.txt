cmake_minimum_required(VERSION 3.20)

add_executable(master_server
    src/main.cpp
    src/registry.cpp
)
target_include_directories(master_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(master_server PRIVATE nlohmann_json::nlohmann_json httplib)
