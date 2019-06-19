# emscripten-sdl2-ogles2
*OpenGL to WebGL using Emscripten*

Demonstrates the basics of porting desktop graphics to the web using Emscripten, via a collection of C++/SDL/OpenGL samples. Specifically, code written in C++, SDL2, and OpenGLES2 is transpiled into Javascript and WebGL by Emscripten ([source](https://github.com/erik-larsen/emscripten-sdl2-ogles2/blob/master/src/)).

### [Run Hello Triangle](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_triangle.html) 

![Hello Triangle](media/hello_triangle.png)

Demonstrates a colorful triangle using shaders, with support for mouse and touch input:
 * Pan using left mouse or finger drag.
 * Zoom using mouse wheel or pinch gesture.

### [Run Hello Texture](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_texture.html)

![Hello Texture](media/hello_texture.png)

Demonstrates a textured triangle, using SDL to load an image from a file.

### [Run Hello Text](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_text_ttf.html)

![Hello Text](media/hello_text_ttf.png)

Demonstrates TrueType text, using SDL to render a string into a texture and apply it to a quad.

### [Run Hello Texture Atlas](https://erik-larsen.github.io/emscripten-sdl2-ogles2/hello_text_txf.html)

![Hello Texture Atlas](media/hello_text_txf.png)

Demonstrates SGI's Texfont text, loading a font texture atlas from a .txf file and applying it to a quad, as well as rendering of text strings.

## Motivation

### Why Emscripten?  

For users, running an app in the browser is the ultimate convenience: nNo need to manually install anything, and the app can run equally well on desktop, tablet, and phone.  For developers, Emscripten does the work to produce optimal Javascript/WASM, replacing the manual, boring, and error-prone process of porting code.

### Why SDL2? 

These demos require OS-dependent stuff (keyboard, mouse, touch, text, audio, networking, etc.). SDL provides a cross-platform library to access this.

### Why OpenGLES2?  

WebGL is the way to get GPU-accelerated graphics in the browser, and OpenGLES is the subset of OpenGL which most closely matches WebGL.