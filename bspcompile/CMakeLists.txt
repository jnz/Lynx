# Project name is not mandatory, but you should use it
project(lynx3d)
 
# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(WIN32)
    TARGET_LINK_LIBRARIES(bspcompile winmm)
ENDIF(WIN32)

link_libraries (
)

set(bspcompile_SOURCES bspcompile.cpp ../lynx/BSPTree.cpp ../lynx/lynx.cpp ../lynx/math/matrix.cpp ../lynx/math/plane.cpp ../lynx/math/quaternion.cpp ../lynx/math/vec3.cpp)

add_executable(bspcompile ${bspcompile_SOURCES})


