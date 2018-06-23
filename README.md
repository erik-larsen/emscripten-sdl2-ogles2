# emscripten-sdl2-ogles2
*C++/SDL2/OpenGLES2 to Javascript/WebGL sample*

This project is a sample/testbed for building graphical apps that run in the browser.  The motivation for this is because I want to preserve a bunch of old graphics demos written in C/C++.  By getting them running in the browser and making the source open to everyone, I hope these demos can live on forever.

Perhaps this can one day grow into a more general library for building C/C++ graphics demos that run in the browser (as well as natively in Mac, Windows, and Linux), but for now the goal is to preserve these old demos.

## Why C++ and Emscripten?  

Because these old demos are written in C/C++.  Better to have Emscripten transpile these automatically to optimized Javascript/WASM than doing the boring and error-prone work of hand porting them.  There will be some porting work to massage this old code into cross-platform C/C++, but that should be much easier than porting to native Javascript.

## Why SDL2? 

Because these old demos need OS event handling (keyboard, mouse, touch, text, audio, networking, etc.).  SDL provides a cross-platform library to do this and Emscripten supports SDL.  A good number of Steam games already utilize SDL, so this seems to be a good choice.  

## Why OpenGLES2?  

Because these old demos require GPU accelerated graphics in the browser, and WebGL is the way to get that.  OpenGLES specifically, because it is the subset of OpenGL which maps to WebGL, and is what Emscripten can transpile directly to WebGL.


