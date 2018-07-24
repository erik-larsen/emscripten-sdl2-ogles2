emcc hello_text_ttf.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 -s FULL_ES2=1 -s WASM=0 --preload-file LiberationSansBold.ttf -o hello_text_ttf_debug.html
emcc hello_text_txf.cpp texfont.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=0 --preload-file rockfont.txf -o hello_text_txf_debug.html
emcc hello_texture.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s FULL_ES2=1 -s WASM=0 --preload-file texmap.png -o hello_texture_debug.html
emcc hello_triangle.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=0 -o hello_triangle.js