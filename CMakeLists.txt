cmake_minimum_required(VERSION 3.12)
project(fps LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

find_package(nlohmann_json 3.2.0 REQUIRED)

set(CLIENT_SOURCES src/client.cpp src/Wall.cpp src/Bullet.cpp src/Player.cpp src/Game.cpp)    
add_executable(${PROJECT_NAME}-server src/server.cpp)
add_executable(${PROJECT_NAME}-client ${CLIENT_SOURCES})

target_link_libraries(${PROJECT_NAME}-client PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME}-client PRIVATE sfml-graphics sfml-network nlohmann_json::nlohmann_json)