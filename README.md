# emscripten-sdl2-ogles2
*C++/SDL2/OpenGLES2 to Javascript/WebGL using Emscripten*

This project is a sample/testbed for porting C++/OpenGL graphical apps to run in the browser.  The motivation for this is to preserve a bunch of old graphics demos written in C/C++.  By getting these demos running in the browser and making them open source, hopefully they can live on for a very long time.  This project is inspired by similar work which uses Emscripten to [preserve arcade games in the browser](https://archive.org/details/internetarcade).

## Try it

[Hello Triangle](https://erik-larsen.github.io/emscripten-sdl2-ogles2/) - Demonstrates minimal code needed, with support for mouse and touch input (see also [debug version](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_triangle_debug.html)).

## Why Emscripten?  

Because these demos should run in the browser and porting to JS is no fun.  Running an app in the browser is the ultimate convenience for the user (nothing to download and install).  Better to have Emscripten do this, producing optimal Javascript/WASM, than doing the boring and error-prone work of hand porting code.  Some porting work will still be necessary, but only to port into cross-platform C/C++.

## Why SDL2? 

Because these demos need OS event handling (keyboard, mouse, touch, text, audio, networking, etc.) and SDL provides a cross-platform library to do this.  And Emscripten supports SDL.

## Why OpenGLES2?  

Because these demos require GPU accelerated graphics. Using the GPU in the browser requires WebGL, and OpenGLES is the path to WebGL via Emscripten.

Perhaps this project could one day grow into a library for building C/C++ graphics demos that run in the browser, but for now the goal is to preserve these old demos.
