# Project name is not mandatory, but you should use it
project(lynx3d)
 
# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

# Mac OS X
IF(APPLE)
   INCLUDE_DIRECTORIES("/usr/local/include")
   LINK_DIRECTORIES("/usr/local/lib")
ENDIF(APPLE)

set(lynx3d_MATH ../src/math/matrix.cpp ../src/math/plane.cpp 
                ../src/math/quaternion.cpp ../src/math/vec3.cpp)

set(bspcompile_SOURCES bspcompile.cpp 
    ../src/BSPTree.cpp ../src/lynx.cpp)

add_executable(bspcompile ${bspcompile_SOURCES} ${lynx3d_MATH})
