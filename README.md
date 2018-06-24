# emscripten-sdl2-ogles2
*C++/SDL2/OpenGLES2 to Javascript/WebGL sample*

This project is a sample/testbed for porting C++/OpenGL graphical apps to run in the browser.  The motivation for this is to preserve a bunch of old graphics demos written in C/C++.  By getting them running in the browser and making them open source, hopefully these demos can live on for a very long time.  This work was inspired by similar work to preserve arcade games in the browser at https://archive.org/details/internetarcade.

## Why C++ and Emscripten?  

Because these demos are written in C/C++ and running them in a browser is the ultimate convenience for the user.  The more convenient, the more likely they can live on.  Better to have Emscripten transpile these into optimized Javascript/WASM than doing the boring and error-prone work of hand porting them.  Some porting work will still be necessary, to massage this old code into cross-platform C/C++, but that should be far less work than porting to Javascript.

## Why SDL2? 

Because these old demos need OS event handling (keyboard, mouse, touch, text, audio, networking, etc.) and SDL provides a cross-platform library to do this.  And Emscripten supports SDL.

## Why OpenGLES2?  

Because these old demos require GPU accelerated graphics. Using the GPU in the browser requires WebGL, and OpenGLES is the path to WebGL via Emscripten.

Perhaps this project could one day grow into a library for building C/C++ graphics demos that run in the browser, but for now the goal is to preserve these old demos.
