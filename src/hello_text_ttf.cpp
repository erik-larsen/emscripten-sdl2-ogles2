//
// Emscripten/SDL2/OpenGLES2 sample that displays TrueType text by loading a font and building a string texture 
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc -std=c++11 hello_text_ttf.cpp events.cpp camera.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 -s FULL_ES2=1 -s WASM=0 --preload-file media/LiberationSansBold.ttf -o hello_text_ttf.html
// 
// Run:
//     emrun hello_text_ttf.html
//
// Result:
//     A TTF text quad and colorful triangle.  Left mouse pans, mouse wheel zooms in/out.
//

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_opengles2.h>

#include "events.h"
EventHandler eventHandler("Hello TTF Text");

// Geometry
GLuint triangleVbo = 0;
GLuint quadVbo = 0;

// Texture
GLuint textureObj = 0;

// Text
const char* cFontName = "media/LiberationSansBold.ttf";
const int cFontPointSize = 64;
const char* message = "Hello Text";

// Shader vars
const GLint positionAttrib = 0;
GLint shaderPan, shaderZoom, shaderAspect, shaderViewport, shaderTextSize, shaderTexSize;
GLfloat textSize[2] = {0.0f, 0.0f}, texSize[2] = {0.0f, 0.0f};

// Text quad vertex & fragment shaders
GLuint quadShaderProgram = 0;
const GLchar* quadVertexSource =
    "attribute vec4 position;                                   \n"
    "varying vec2 texCoord;                                     \n"
    "uniform vec2 viewport;                                     \n"
    "uniform vec2 textSize;                                     \n"
    "uniform vec2 texSize;                                      \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_Position = vec4(position.xyz, 1.0);                 \n"
    "    gl_Position.x *= textSize.x;                           \n"
    "    gl_Position.y *= textSize.y;                           \n"
    "                                                           \n"
    "    // Translate to lower left viewport                    \n"
    "    gl_Position.x -= viewport.x / 2.0;                     \n"
    "    gl_Position.y -= viewport.y / 2.0;                     \n"
    "                                                           \n"
    "    // Ortho projection                                    \n"
    "    gl_Position.x += 1.0;                                  \n"
    "    gl_Position.x *= 2.0 / viewport.x;                     \n"
    "    gl_Position.y += 1.0;                                  \n"
    "    gl_Position.y *= 2.0 / viewport.y;                     \n"
    "                                                           \n"
    "    // Text subrectangle from overall texture              \n"
    "    texCoord.x = position.x * textSize.x / texSize.x;      \n"
    "    texCoord.y = -position.y * textSize.y / texSize.y;     \n"
    "}                                                          \n";

const GLchar* quadFragmentSource =
    "precision mediump float;                                   \n"
    "varying vec2 texCoord;                                     \n"
    "uniform sampler2D texSampler;                              \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_FragColor = texture2D(texSampler, texCoord);        \n"
    "}                                                          \n";

// Colorful triangle vertex & fragment shaders
GLuint triShaderProgram = 0;
const GLchar* triVertexSource =
    "uniform vec2 pan;                             \n"
    "uniform float zoom;                           \n"
    "uniform float aspect;                         \n"
    "attribute vec4 position;                      \n"
    "varying vec3 color;                           \n"
    "void main()                                   \n"
    "{                                             \n"
    "    gl_Position = vec4(position.xyz, 1.0);    \n"
    "    gl_Position.xy += pan;                    \n"
    "    gl_Position.xy *= zoom;                   \n"
    "    gl_Position.y *= aspect;                  \n"
    "    color = gl_Position.xyz + vec3(0.5);      \n"
    "}                                             \n";

const GLchar* triFragmentSource =
    "precision mediump float;                     \n"
    "varying vec3 color;                          \n"
    "void main()                                  \n"
    "{                                            \n"
    "    gl_FragColor = vec4 ( color, 1.0 );      \n"
    "}                                            \n";

int nextPowerOfTwo(int val)
{
    int power = 1;
    while (power < val)
        power *= 2;
    return power;
}

void updateShader()
{
    Camera& camera = eventHandler.camera();

    glUseProgram(quadShaderProgram);
    glUniform2fv(shaderViewport, 1, camera.viewport());
    glUniform2fv(shaderTextSize, 1, textSize);
    glUniform2fv(shaderTexSize, 1, texSize);

    glUseProgram(triShaderProgram);
    glUniform2fv(shaderPan, 1, camera.pan());
    glUniform1f(shaderZoom, camera.zoom()); 
    glUniform1f(shaderAspect, camera.aspect());
}

GLuint initShader(const GLchar* vertexSource, const GLchar* fragmentSource)
{
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link vertex and fragment shader into shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindAttribLocation(shaderProgram, positionAttrib, "position");
    glEnableVertexAttribArray(positionAttrib);
    glLinkProgram(shaderProgram);

    return shaderProgram;
}

void initShaders()
{
    // Compile & link shaders
    quadShaderProgram = initShader(quadVertexSource, quadFragmentSource);
    triShaderProgram = initShader(triVertexSource, triFragmentSource);

    // Get shader variables and initalize them
    shaderViewport = glGetUniformLocation(quadShaderProgram, "viewport");
    shaderTextSize = glGetUniformLocation(quadShaderProgram, "textSize");
    shaderTexSize = glGetUniformLocation(quadShaderProgram, "texSize");

    shaderPan = glGetUniformLocation(triShaderProgram, "pan");
    shaderZoom = glGetUniformLocation(triShaderProgram, "zoom");    
    shaderAspect = glGetUniformLocation(triShaderProgram, "aspect");
    
    updateShader();
}

void initGeometry()
{
   // Create vertex buffer objects and copy vertex data into them
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    GLfloat quadVertices[] = 
    {
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    GLfloat triangleVertices[] = 
    {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);  
 }

void debugPrintSurface(SDL_Surface* surface, const char* name, bool dumpPixels)
{
    printf ("%s dimensions %dx%d, %d bits per pixel\n", name, surface->w, surface->h, surface->format->BitsPerPixel);
    if (dumpPixels)
    {
        for (int i = 0; i < surface->w * surface->h; ++i)
            printf("%x ", ((unsigned int*)surface->pixels)[i]);
        printf("\n");
    }
}

void initTextTexture()
{
    TTF_Init();

    // Load the font
    TTF_Font *font = TTF_OpenFont(cFontName, cFontPointSize);
    if (font) 
    {
        // Render text to surface
        SDL_Color foregroundColor = {255,255,255,255};
        SDL_Surface* textImage8Bit = TTF_RenderText_Solid(font, message, foregroundColor);
        
        if (textImage8Bit)
        {
            // Convert surface from 8 to 32 bit for GL
            SDL_Surface* textImage = SDL_ConvertSurfaceFormat(textImage8Bit, SDL_PIXELFORMAT_RGBA8888, 0);
            debugPrintSurface(textImage, "textImage", false);

            // Create power of 2 dimensioned texture for GL with 1 texel border, clear it, and copy text image into it
            SDL_Surface* texture = SDL_CreateRGBSurface(0, nextPowerOfTwo(textImage->w + 2), nextPowerOfTwo(textImage->h + 2), 
                                                        textImage->format->BitsPerPixel, 0, 0, 0, 0);
            memset(texture->pixels, 0x0, texture->w * texture->h * texture->format->BytesPerPixel);
            SDL_Rect destRect = {1, texture->h - textImage->h - 1, textImage->w + 1, texture->h - 1};
            SDL_SetSurfaceBlendMode(textImage, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(textImage, NULL, texture, &destRect);
 
            // Emscripten/SDL bug? SDL_BlitSurface should copy source alpha when source surface is set to 
            // SDL_BLENDMODE_NONE, however this is not happening, so fix it up here
            unsigned int* pixels = (unsigned int*)texture->pixels;
            for (int i = 0; i < texture->w*texture->h; ++i)
            {
                if (pixels[i] != 0)
                    pixels[i] |= 0xff000000;
                else
                    pixels[i] = 0x80808080;
            }
            debugPrintSurface(texture, "texture", false);

            // Determine GL texture format
            GLint format = -1;
            if (texture->format->BitsPerPixel == 24)
                format = GL_RGB;
            else if (texture->format->BitsPerPixel == 32)
                format = GL_RGBA;

            if (format != -1)
            {
                // Enable blending for texture alpha component
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // Generate a GL texture object
                glGenTextures(1, &textureObj);

                // Bind GL texture
                glBindTexture(GL_TEXTURE_2D, textureObj);

                // Set the GL texture's wrapping and stretching properties
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                // Copy SDL surface image to GL texture
                glTexImage2D(GL_TEXTURE_2D, 0, format, 
                             texture->w, texture->h, 
                             0, format, GL_UNSIGNED_BYTE, texture->pixels);

                // Update quad shader
                texSize[0] = (GLfloat)texture->w;
                texSize[1] = (GLfloat)texture->h;
                textSize[0] = (GLfloat)textImage->w + 2;
                textSize[1] = (GLfloat)textImage->h + 2;
                updateShader();
            }
                                    
            SDL_FreeSurface (textImage);        
            SDL_FreeSurface (texture);        
        }  
        TTF_CloseFont(font);
    }
    else
        printf("Failed to load font %s, due to %s\n", cFontName, TTF_GetError());
}

void redraw()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle VBO with a colorful shader
    glUseProgram(triShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Draw the quad VBO with a text texture shader
    glUseProgram(quadShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Swap front/back framebuffers
    eventHandler.swapWindow();
}

void mainLoop() 
{    
    eventHandler.processEvents();

    // Update shader if camera changed
    if (eventHandler.camera().updated())
        updateShader();

    redraw();
}

int main(int argc, char** argv)
{
    // Initialize graphics
    initShaders();
    initGeometry();
    initTextTexture();

    // Start the main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, true);
#else
    while(true) 
        mainLoop();
#endif

    glDeleteTextures(1, &textureObj);

    return 0;
}