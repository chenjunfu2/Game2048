cmake_minimum_required(VERSION 4.2)
project(Game2048)

#C++ 20
set(CMAKE_CXX_STANDARD 20)

#debug define
#add_definitions(-D_DEBUG)

#exec
add_executable(Game2048 Game2048/main.cpp)
