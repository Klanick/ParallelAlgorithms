project(lab2)

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

find_package(Parlay REQUIRED)
add_executable(lab2 main.cpp)
target_link_libraries(lab2 PUBLIC Parlay::parlay)


