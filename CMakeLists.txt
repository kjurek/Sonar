cmake_minimum_required(VERSION 2.8.12)

project(Sonar)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)

add_executable(${PROJECT_NAME} main.cpp process_manager.cpp process_manager.h player.h sonar.h sonar.cpp)

target_link_libraries(${PROJECT_NAME} psapi)
