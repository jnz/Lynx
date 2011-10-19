project(lynx3d)

# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

IF(UNIX)
    IF(APPLE) # Mac OS X
        INCLUDE_DIRECTORIES("/usr/local/include")
        LINK_DIRECTORIES("/usr/local/lib")
    ELSE(APPLE) # GNU c++0x flags are used here
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x") # for unordered_map
    ENDIF(APPLE)
ENDIF(UNIX)

# Include the directory itself as a path to include directories
# set(CMAKE_INCLUDE_CURRENT_DIR ON)

# REQUIRED does not work in CMake <=2.4.6 for SDL
Find_Package ( SDL REQUIRED )
Find_Package ( SDL_mixer REQUIRED )

# Workaround for the non-working REQUIRED flag
if ( NOT SDL_FOUND )
    message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )

FIND_PATH( GLEW_INCLUDE_DIR glew.h wglew.h
    PATHS /usr/local/include /usr/include
    PATH_SUFFIXES gl/ GL/ )
SET( GLEW_NAMES glew GLEW )
FIND_LIBRARY( GLEW_LIBRARY
    NAMES ${GLEW_NAMES}
    PATHS /usr/lib /usr/local/lib )

find_package(OpenGL REQUIRED)

IF(WIN32)
    TARGET_LINK_LIBRARIES(lynx3d ws2_32 winmm opengl32 glu32)
ENDIF(WIN32)

link_libraries (
    enet
    ${OPENGL_LIBRARIES}
    ${SDL_LIBRARY}
    ${SDLMIXER_LIBRARY}
    ${GLEW_LIBRARY}
    SDLmain # Sadly not included in SDL_LIBRARY variable
)

# the external image loading lib SOIL
set(lynx3d_SOIL ../soil/src/SOIL.c
                ../soil/src/image_DXT.c
                ../soil/src/image_helper.c
                ../soil/src/stb_image_aug.c)

set(lynx3d_MATH math/matrix.cpp math/plane.cpp math/quaternion.cpp math/vec3.cpp)

set(lynx3d_SOURCES BSPLevel.cpp Client.cpp ClientHUD.cpp ClientInfo.cpp
    Frustum.cpp GameLogic.cpp GameObj.cpp GameObjPlayer.cpp GameObjZombie.cpp
    GameObjRocket.cpp GameZombie.cpp Mixer.cpp NetMsg.cpp Obj.cpp
    ParticleSystem.cpp ParticleSystemBlood.cpp ParticleSystemExplosion.cpp
    ParticleSystemDust.cpp ParticleSystemRocket.cpp Renderer.cpp
    ResourceManager.cpp Server.cpp Stream.cpp Sound.cpp Think.cpp World.cpp
    WorldClient.cpp lynx.cpp ModelMD5.cpp lynxsys.cpp Menu.cpp Font.cpp
    Config.cpp Model.cpp ModelMD2.cpp main.cpp)

set(lynx3dsv_SOURCES BSPLevel.cpp ClientHUD.cpp ClientInfo.cpp Frustum.cpp GameLogic.cpp
    GameObj.cpp GameObjPlayer.cpp GameObjZombie.cpp GameZombie.cpp
    GameObjRocket.cpp NetMsg.cpp Obj.cpp ParticleSystem.cpp
    ParticleSystemBlood.cpp ParticleSystemDust.cpp ParticleSystemExplosion.cpp
    ParticleSystemRocket.cpp ResourceManager.cpp Server.cpp Sound.cpp
    Stream.cpp Think.cpp World.cpp ModelMD5.cpp lynx.cpp lynxsys.cpp Config.cpp
    Model.cpp ModelMD2.cpp mainsv.cpp)

add_executable(lynx3d ${lynx3d_SOURCES} ${lynx3d_MATH} ${lynx3d_SOIL})
add_executable(lynx3dsv ${lynx3dsv_SOURCES} ${lynx3d_MATH} ${lynx3d_SOIL})

