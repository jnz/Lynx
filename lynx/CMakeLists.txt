# Project name is not mandatory, but you should use it
project(lynx3d)
 
# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(lynx3d_MATH math/matrix.cpp math/plane.cpp math/quaternion.cpp math/vec3.cpp)
set(lynx3d_SOURCES BSPLevel.cpp Client.cpp ClientHUD.cpp ClientInfo.cpp Frustum.cpp GameLogic.cpp GameObj.cpp GameObjPlayer.cpp GameObjZombie.cpp GameZombie.cpp ModelMD2.cpp NetMsg.cpp Obj.cpp ParticleSystem.cpp ParticleSystemBlood.cpp ParticleSystemDust.cpp Renderer.cpp ResourceManager.cpp Server.cpp Stream.cpp Think.cpp World.cpp WorldClient.cpp lynx.cpp lynxsys.cpp main.cpp)

add_executable(lynx3d ${lynx3d_SOURCES} ${lynx3d_MATH})

