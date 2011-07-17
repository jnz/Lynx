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

External Libraries
==================

Download the following libraries:

enet: [http://enet.bespin.org/download/enet-1.2.tar.gz](http://enet.bespin.org/download/enet-1.2.tar.gz)
SDL: [http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip](http://www.libsdl.org/release/SDL-devel-1.2.13-VC8.zip)
glew: [http://glew.sourceforge.net/](http://glew.sourceforge.net/)

ENet
----

Extract enet to a directory and open the "enet.dsp" project. Compile the enet
library. Add library and include files to your VS path: Tools -> Options ->
"Projects and Solutions" -> "VC++ Directories". Select "library files" and add a
new line and select the enet "lib" folder. Then goto "include files" and add a
new line for the enet "include" directory.

SDL
---

Extract SDL to a directory (you don't need to compile SDL) and add the include and lib directory in the same way as for enet. Copy the lib\SDL.dll to your lynx3d\lynx directory.

Glew
----

Same with glew.
