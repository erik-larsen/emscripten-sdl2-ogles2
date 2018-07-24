# emscripten-sdl2-ogles2
*C++/SDL2/OpenGLES2 to Javascript/WebGL using Emscripten*

This project is a collection of C++/SDL2/OpenGL samples that run in the browser via Emscripten.  These samples can serve as building blocks to aid in porting C++ graphics apps to the browser.

The long-term goal with this work is to preserve old graphics demos, particularly those developed in the 1990s by Silicon Graphics.  This work is inspired in part by the [preservation of classic arcade games in the browser](https://archive.org/details/internetarcade), which also utilizes Emscripten.


## Try it!

### [Hello Triangle](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_triangle.html)

![Hello Triangle](media/hello_triangle.png)

Demonstrates a colorful triangle using shaders, with support for mouse and touch input.  

Controls: 
- Pan with left mouse or finger touch
- Zoom with mouse wheel or pinch gesture.

### [Hello Texture](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_texture_debug.html)

![Hello Texture](media/hello_texture.png)

Demonstrates a textured triangle, using SDL to load an image from a file.

### [Hello Text](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_text_ttf_debug.html)

![Hello Text](media/hello_text_ttf.png)

Demonstrates TrueType text on a quad, using SDL to render a string into a texture.

### [Hello Texture Atlas](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_text_txf_debug.html)

![Hello Texture Atlas](media/hello_text_txf.png)

Demonstrates Texfont text on a quad, loading a font texture atlas from a .txf font file.  


## Why Emscripten?  

Running an app in the browser is the ultimate convenience for the user - no manual download and install is necessary and it will run on desktop, tablet, and phone.  Better to have Emscripten do the work to produce optimal Javascript/WASM, than doing the boring and error-prone work of hand porting C++ code.  Some porting work will still be necessary, but only to port into cross-platform C++.

## Why SDL2? 

These demos require OS-dependent stuff (keyboard, mouse, touch, text, audio, networking, etc.). SDL provides a cross-platform library to access this, and Emscripten has an SDL port.

## Why OpenGLES2?  

These demos require GPU accelerated graphics. Using the GPU in the browser requires WebGL, and OpenGLES is the path to generating WebGL code via Emscripten.
