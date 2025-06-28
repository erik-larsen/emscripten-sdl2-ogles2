:: Requires Emscripten to build: https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
call emcc -std=c++11 hello_triangle_min.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=1 -o web\hello_triangle_min.js
call emcc -std=c++11 -DEVENTS_DEBUG=1 hello_triangle.cpp events.cpp camera.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=1 -o web\hello_triangle.js
call emcc -std=c++11 hello_texture.cpp events.cpp camera.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS="[""png""]" -s FULL_ES2=1 -s WASM=1 --preload-file media/texmap.png -o web\hello_texture.js
call emcc -std=c++11 hello_text_ttf.cpp events.cpp camera.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 -s FULL_ES2=1 -s WASM=1 --preload-file media/LiberationSansBold.ttf -o web\hello_text_ttf.js
call emcc -std=c++11 hello_text_txf.cpp events.cpp camera.cpp texfont.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=1 --preload-file media/rockfont.txf -o web\hello_text_txf.js
call emcc -std=c++11 hello_image.cpp events.cpp camera.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s FULL_ES2=1 -s WASM=1 -o web\hello_image.js
