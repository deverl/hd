cmake_minimum_required(VERSION 4.2)
project(hd_project)

set(CMAKE_CXX_STANDARD 20)

add_executable(hd hd.cpp)

target_compile_options(hd PRIVATE -Wall -O2)

