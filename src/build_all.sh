# Requires Emscripten to build: https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
emcc hello_triangle.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=0 -o ../hello_triangle.js
emcc hello_texture.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s FULL_ES2=1 -s WASM=0 --preload-file media/texmap.png -o ../hello_texture.js
emcc hello_text_ttf.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 -s FULL_ES2=1 -s WASM=0 --preload-file media/LiberationSansBold.ttf -o ../hello_text_ttf.js
emcc hello_text_txf.cpp texfont.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=0 --preload-file media/rockfont.txf -o ../hello_text_txf.js
