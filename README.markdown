Lynx
====

Lynx is a simple 3D FPS with zombies :)
If zombies won't convince you, there are some other features:

- Multiplayer with robust smooth network latency interpolation
- Network compression (delta compression and PPM range coder)
- Multiplatform support (Windows, Mac OS X, Linux)
- OpenGL 2.0
- Per-pixel lighting
- Shadow mapping (dynamic)
- Light mapping (precalculated)
- Normal mapping
- MD2 and MD5 mesh support
- MD2 interpolation on hardware (allows to render tons of enemys)
- KD-tree compiler for collision detection on complex geometry
- Blender as a map editor
- Dedicated server mode
- Did I mention zombies?

![lynx][1]

Dependencies
============

Software
--------

On Linux and Mac OS X, the project is using cmake to generate
a build environment for the project. On Windows, you can use
the Visual Studio project files.

- cmake [http://www.cmake.org/](http://www.cmake.org/)


External Libraries
------------------

Download and install the following libraries:

- SDL 1.2.x Development Library: [http://www.libsdl.org/download-1.2.php](http://www.libsdl.org/download-1.2.php)
- GLEW Library: [http://glew.sourceforge.net/](http://glew.sourceforge.net/)

SDL is used for the cross-platform low level access to audio, keyboard, mouse
and 3D hardware.
GLEW is the OpenGL Extension Wrangler Library and is used to access OpenGL
extensions.

Compile Lynx on Windows
=======================

Use the Visual Studio 2010 project file

> lynx.sln

And compile the project. Make sure you have the libraries in path.

Compile Lynx on Linux and Mac OS X
==================================

Download Lynx from Github

> git clone https://github.com/jnz/Lynx.git lynx

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

If you have a lightmap available, you need to place the same geometry in a
lightmap.obj file, but with lightmap texture coordinates.
The lightmap itself is stored in the *lightmap.jpg* texture.

Currently I am using the *gile[s]* radiosity lightmapper
to create lightmaps.
Link: [http://www.frecle.net/index.php?show=giles.about](http://www.frecle.net/index.php?show=giles.about)

[1]: http://www.zwiener.org/pics/lynx3d/lynx_md5.png
