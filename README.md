# emscripten-sdl2-ogles2
*C++/SDL2/OpenGLES2 to Javascript/WebGL using Emscripten*

This project is a collection of simple C++/SDL2/OpenGL graphical samples that run in the browser via Emscripten.  The long-term goal with this work is to preserve old OpenGL graphics demos, particularly those developed in the 1990s by Silicon Graphics.  The immediate goal however is to build up a collection of small samples that can serve as building blocks for porting these old demos to the browser.

With these demos running in the browser, hopefully they can live on for a long time as early examples of interactive 3D graphics.  This work is inspired in part by the [preservation of classic arcade games in the browser](https://archive.org/details/internetarcade), which also utilizes Emscripten.

## Try it

[Hello Triangle](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_triangle.html) - Demonstrates minimal code needed to draw a colorful triangle with shaders, with support for mouse and touch input.  Controls: 
- Pan with left mouse or finger touch
- Zoom with mouse wheel or pinch gesture.

[Hello Texture](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_texture_debug.html) - Similar to Hello Triangle, but the triangle is textured with an image file.

[Hello Text](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_ttf_text_debug.html) - Similar tp Hello Triangle, but the triangle contains text rendered using a TrueType font.

## Why Emscripten?  

These demos need to run in the browser and porting to Javascript is no fun.  Running an app in the browser is the ultimate convenience for the user (nothing to download and install, works on desktop, tablet, and phone).  Better to have Emscripten do the work to produce optimal Javascript/WASM, than doing the boring and error-prone work of hand porting code.  Some porting work will still be necessary, but only to port into cross-platform C++.

## Why SDL2? 

These demos require OS-dependent stuff (keyboard, mouse, touch, text, audio, networking, etc.), SDL provides a cross-platform library to access this, and Emscripten runs SDL in the browser.

## Why OpenGLES2?  

These demos require GPU accelerated graphics. Using the GPU in the browser requires WebGL, and OpenGLES is the path to generating WebGL code via Emscripten.
