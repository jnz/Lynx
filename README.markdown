Lynx
====

Lynx is a simple 3D FPS with zombies :)
If zombies won't convince you, there are some other features:

- Multiplayer (UDP only)
- Shadow mapping, per-pixel lighting
- Dedicated server
- Multiplatform support (Windows, Mac OS X, Linux)
- Blender as a map editor + custom bsp compiler
- OpenGL 2.0
- Did I mention zombies?

Dependencies
============

Software
--------

- cmake [http://www.cmake.org/](http://www.cmake.org/)

External Libraries
------------------

Download and install the following libraries:

- enet: [http://enet.bespin.org/download/enet-1.2.tar.gz](http://enet.bespin.org/download/enet-1.2.tar.gz)
- SDL: [http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip](http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip)
- glew: [http://glew.sourceforge.net/](http://glew.sourceforge.net/)

Compile Lynx on Linux and Mac OS X
==================================

Download Lynx from Github
> git clone https://jnz@github.com/jnz/Lynx.git lynx
> cd lynx

Create build directory and run CMake
> mkdir build
> cmake ../
> make

This will create the following executable files:
> lynx/build/src/lynx3dsv (dedicated server)
> lynx/build/src/lynx3d (game client and server)

Copy these files (lynx3d and lynx3dsv) to the game directory:
> lynx/game

The game folder contains the *baselynx* folder, where all the resources are
stored.

Run Lynx
========

Local game:

> lynx/game/lynx3d

Connect to remote server with the ip address 192.168.0.1 at port 9999:

> lynx/game/lynx3d 192.168.0.1 9999

Start a dedicated server at port 9999:

> lynx/game/lynx3dsv 9999

