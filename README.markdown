Lynx
====

Lynx is a simple 3D FPS with zombies :)
If zombies won't convince you, there are some other features:

- Multiplayer with robust network latency interpolation
- OpenGL 2.0
- Shadow mapping
- Per-pixel lighting
- Normal mapping
- Dedicated server
- Multiplatform support (Windows, Mac OS X, Linux)
- Blender as a map editor + custom KD tree compiler
- Did I mention zombies?

Dependencies
============

Software
--------

- cmake [http://www.cmake.org/](http://www.cmake.org/)

External Libraries
------------------

Download and install the following libraries:

- enet: [http://enet.bespin.org/SourceDistro.html](http://enet.bespin.org/SourceDistro.html)
- SDL: [http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip](http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip)
- glew: [http://glew.sourceforge.net/](http://glew.sourceforge.net/)

Compile Lynx on Linux and Mac OS X
==================================

Download Lynx from Github

> git clone https://jnz@github.com/jnz/Lynx.git lynx

> cd lynx

Create a build directory in the lynx folder

> mkdir build

> cd build

Run CMake from the build/ folder

> cmake ../

This will create a Makefile

> make

This will create the client executable *lynx3d*

> lynx/build/src/lynx3d (game client and server)

And a dedicated server module *lynx3dsv*

> lynx/build/src/lynx3dsv (dedicated server)

Copy these files (lynx3d and lynx3dsv) to the game directory:

> lynx/game

The game folder contains the *baselynx* folder, where all the resources are
stored.

Run Lynx
========

Local game:

> lynx3d

Connect to remote server with the ip address 192.168.0.1 at port 9999:

> lynx3d 192.168.0.1 9999

Start a dedicated server at port 9999:

> lynx3dsv 9999

Create a level in Lynx
======================

You need to have your geometry available as Wavefront .obj file with embedded
texture coordinates. Make sure the face normals are exported too.
The usemtl keyword in the .obj file points directly to the texture file. No .mtl
files are supported.
Then you need to convert this .obj file to a .lbsp file with the kdcompile
helper program.  kdcompile reads the .obj file and writes a Lynx compatible
.lbsp file.

> kdcompile mylevel.obj mylevel.lbsp

