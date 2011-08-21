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

set(kdcompile_SOURCES kdcompile.cpp ../src/KDTree.cpp ../src/lynx.cpp)

add_executable(kdcompile ${kdcompile_SOURCES} ${lynx3d_MATH})
