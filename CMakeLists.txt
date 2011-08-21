# Project name is not mandatory, but you should use it
project(lynx3d)

# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE DEBUG)

add_subdirectory(src)
add_subdirectory(kdcompile)

