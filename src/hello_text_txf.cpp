//
// Emscripten/SDL2/OpenGLES2 sample that displays Texfont text by loading a font and building a texture atlas
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc -std=c++11 hello_text_txf.cpp events.cpp camera.cpp texfont.cpp -s USE_SDL=2 -s FULL_ES2=1 -s WASM=0 --preload-file media/rockfont.txf -o hello_text_txf.html
// 
// Run:
//     emrun hello_text_txf.html
//
// Result:
//     A TXF text quad and colorful triangle.  Left mouse pans, mouse wheel zooms in/out.
//

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_opengles2.h>

#include "events.h"
#include "texfont.h"

// Triangle
GLuint triangleVbo = 0;

// Text
const char* cFontName = "media/rockfont.txf";
TexFont* txf = nullptr;
GLuint fontTextureObj = 0;
GLuint quadFontVbo = 0;
GLuint quadsTextStringVbo = 0;

// Shader vars
const GLint positionAttrib = 0;
GLfloat textSize[2] = {0.0f, 0.0f};
GLint shaderPan, shaderZoom, shaderAspect, shaderViewport, shaderTextSize;

//  Text quad vertex & fragment shaders
GLuint quadFontShaderProgram = 0;
const GLchar* quadFontVertexSource =
    "attribute vec4 position;                                   \n"
    "varying vec2 texCoord;                                     \n"
    "uniform vec2 viewport;                                     \n"
    "uniform vec2 textSize;                                     \n"
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
    "    texCoord.x = position.x;                               \n"
    "    texCoord.y = position.y;                               \n"
    "}                                                          \n";

const GLchar* quadFontFragmentSource =
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

void updateShader(EventHandler& eventHandler)
{
    Camera& camera = eventHandler.camera();

    glUseProgram(quadFontShaderProgram);
    glUniform2fv(shaderViewport, 1, camera.viewport());
    glUniform2fv(shaderTextSize, 1, textSize);

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

void initShaders(EventHandler& eventHandler)
{
    // Compile & link shaders
    quadFontShaderProgram = initShader(quadFontVertexSource, quadFontFragmentSource);
    triShaderProgram = initShader(triVertexSource, triFragmentSource);

    // Get shader variables and initalize them
    shaderViewport = glGetUniformLocation(quadFontShaderProgram, "viewport");
    shaderTextSize = glGetUniformLocation(quadFontShaderProgram, "textSize");

    shaderPan = glGetUniformLocation(triShaderProgram, "pan");
    shaderZoom = glGetUniformLocation(triShaderProgram, "zoom");    
    shaderAspect = glGetUniformLocation(triShaderProgram, "aspect");
    
    updateShader(eventHandler);
}

void initGeometry()
{
   // Create vertex buffer objects and copy vertex data into them
    glGenBuffers(1, &quadFontVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadFontVbo);
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

void initTextTexture(EventHandler& eventHandler)
{
    // Determine GL texture format
    GLint format = GL_RGBA;

    txf = txfLoadFont(cFontName);
    if (txf)
    {
        printf("txf dimensions %dx%d\n", txf->tex_width, txf->tex_height);

        // Enable blending for texture alpha component
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // Generate a GL texture object
        glGenTextures(1, &fontTextureObj);

        // Bind GL texture
        glBindTexture(GL_TEXTURE_2D, fontTextureObj);

        // Set the GL texture's wrapping and stretching properties
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        txf->texobj = fontTextureObj;
        unsigned int* txf_pixels = new unsigned int[txf->tex_width * txf->tex_height];
        for (int i = 0; i < txf->tex_width * txf->tex_height; ++i)
        {
            if (txf->teximage[i])
            {
                unsigned char c = txf->teximage[i];
                txf_pixels[i] = c | c << 8 | c << 16 | c << 24;
            }
            else
                txf_pixels[i] = 0;
        }

        // Copy SDL surface image to GL texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, 
                     txf->tex_width, txf->tex_height, 
                     0, format, GL_UNSIGNED_BYTE, txf_pixels);

        textSize[0] = (GLfloat)txf->tex_width;
        textSize[1] = (GLfloat)txf->tex_height;
        updateShader(eventHandler);

        delete[] txf_pixels;
    }
    else
        printf("error loading txf\n");
}

void redraw(EventHandler& eventHandler)
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle VBO with a colorful shader
    glUseProgram(triShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Draw the quad VBO with a text texture shader
    glUseProgram(quadFontShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, quadFontVbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Swap front/back framebuffers
    eventHandler.swapWindow();
}

void mainLoop(void* mainLoopArg) 
{    
    EventHandler& eventHandler = *((EventHandler*)mainLoopArg);
    eventHandler.processEvents();

    // Update shader if camera changed
    if (eventHandler.camera().updated())
        updateShader(eventHandler);

    redraw(eventHandler);
}

int main(int argc, char** argv)
{
    EventHandler eventHandler("Hello TXF Text");

    // Initialize graphics
    initShaders(eventHandler);
    initGeometry();
    initTextTexture(eventHandler);

    // Start the main loop
    void* mainLoopArg = &eventHandler;

#ifdef __EMSCRIPTEN__
    int fps = 0; // Use browser's requestAnimationFrame
    emscripten_set_main_loop_arg(mainLoop, mainLoopArg, fps, true);
#else
    while(true) 
        mainLoop(mainLoopArg);
#endif

    txfUnloadFont(txf);
    glDeleteTextures(1, &fontTextureObj);

    return 0;
}