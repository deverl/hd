cmake_minimum_required(VERSION 4.2)
project(hd_project)

# Set default build type if none specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Customize flags for each build type
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")


set(CMAKE_CXX_STANDARD 20)

add_executable(hd hd.cpp)

target_compile_options(hd PRIVATE -Wall -O2)

