project(lab1)

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

find_package(Parlay REQUIRED)
add_executable(lab1 main.cpp)
target_link_libraries(lab1 PUBLIC Parlay::parlay)


