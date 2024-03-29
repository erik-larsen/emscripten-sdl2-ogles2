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
//     A TXF font quad and colorful triangle.  Left mouse pans, mouse wheel zooms in/out.
//

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_opengles2.h>

#include "events.h"
#include "texfont.h"

// Vertex attribute indices for all shaders
const GLuint vertexPositionIndex = 0, 
             vertexTexCoordIndex = 1;

// Text quads geometry and vertex shader
GLuint quadsTextShaderProgram = 0;
GLint shaderViewport2;
GLint shaderTextureSampler2;

const GLchar* quadsTextVertexSource =
    "uniform vec2 viewport;                                     \n"
    "attribute vec4 position;                                   \n"
    "attribute vec2 texCoord;                                   \n"
    "varying vec2 vTexCoord;                                    \n"    
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_Position = vec4(position.xyz, 1.0);                 \n"
    "                                                           \n"
    "    // Ortho projection                                    \n"
    "    gl_Position.x += 1.0;                                  \n"
    "    gl_Position.x *= 2.0 / viewport.x;                     \n"
    "    gl_Position.y += 1.0;                                  \n"
    "    gl_Position.y *= 2.0 / viewport.y;                     \n"
    "                                                           \n"
    "    vTexCoord = texCoord;                                  \n"  
    "}                                                          \n";

// Font quad texture, geometry, and vertex shader
const char* cFontName = "media/rockfont.txf";
TexFont* texFont = nullptr;
GLuint quadFontVbo = 0;
GLuint quadFontShaderProgram = 0;
GLfloat fontSize[2] = {0.0f, 0.0f};
GLint shaderViewport, shaderFontSize, shaderTextureSampler;
const GLchar* quadFontVertexSource =
    "uniform vec2 viewport;                                     \n"
    "uniform vec2 fontSize;                                     \n"
    "attribute vec4 position;                                   \n"
    "varying vec2 vTexCoord;                                    \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    gl_Position = vec4(position.xyz, 1.0);                 \n"
    "    gl_Position.x *= fontSize.x;                           \n"
    "    gl_Position.y *= fontSize.y;                           \n"
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
    "    vTexCoord.x = position.x;                              \n"
    "    vTexCoord.y = position.y;                              \n"
    "}                                                          \n";

// Font texture fragment shader, shared by text quads and font quad
const GLchar* fontFragmentSource =
    "precision mediump float;                                   \n"
    "uniform sampler2D texSampler;                              \n"
    "varying vec2 vTexCoord;                                    \n"
    "void main()                                                \n"
    "{                                                          \n"
    "    // Text opacity (GL_ALPHA texture)                     \n"
    "    gl_FragColor = texture2D(texSampler, vTexCoord);       \n"
    "                                                           \n"
    "    // Text color white                                    \n"
    "    gl_FragColor.xyz = vec3(1.0, 1.0, 1.0);                \n"
    "}                                                          \n";

// Colorful triangle geometry, vertex & fragment shaders
GLuint triangleVbo = 0;
GLuint triShaderProgram = 0;
GLint shaderPan, shaderZoom, shaderAspect;
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

    glUseProgram(quadsTextShaderProgram);
    glUniform2fv(shaderViewport2, 1, camera.viewport());
    glUniform1i(shaderTextureSampler2, 0);

    glUseProgram(quadFontShaderProgram);
    glUniform2fv(shaderViewport, 1, camera.viewport());
    glUniform2fv(shaderFontSize, 1, fontSize);
    glUniform1i(shaderTextureSampler, 0);

    glUseProgram(triShaderProgram);
    glUniform2fv(shaderPan, 1, camera.pan());
    glUniform1f(shaderZoom, camera.zoom()); 
    glUniform1f(shaderAspect, camera.aspect());
}

GLuint buildShaderProgram(const GLchar* vertexSource, const GLchar* fragmentSource, bool bUseTexCoords)
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
    glBindAttribLocation(shaderProgram, vertexPositionIndex, "position");
    if (bUseTexCoords)
        glBindAttribLocation(shaderProgram, vertexTexCoordIndex, "texCoord");
    
    glLinkProgram(shaderProgram);

    GLenum glError = glGetError();
    if (glError != GL_NO_ERROR)
        printf("ERROR: Shader %d failed to build, error code %d\n", shaderProgram, glError);
    else
        printf("Shader %d built OK.\n", shaderProgram);

    return shaderProgram;
}

void initShaders(EventHandler& eventHandler)
{
    // Compile & link shaders
    quadsTextShaderProgram = buildShaderProgram(quadsTextVertexSource, fontFragmentSource, true);
    quadFontShaderProgram = buildShaderProgram(quadFontVertexSource, fontFragmentSource, false);
    triShaderProgram = buildShaderProgram(triVertexSource, triFragmentSource, false);

    // Get shader uniforms and initialize them
    shaderViewport2 = glGetUniformLocation(quadsTextShaderProgram, "viewport");
    shaderTextureSampler2 = glGetUniformLocation(quadsTextShaderProgram, "texSampler");

    shaderViewport = glGetUniformLocation(quadFontShaderProgram, "viewport");
    shaderFontSize = glGetUniformLocation(quadFontShaderProgram, "fontSize");
    shaderTextureSampler = glGetUniformLocation(quadFontShaderProgram, "texSampler");

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

void initFontTexture(EventHandler& eventHandler)
{
    texFont = txfLoadFont(cFontName);
    if (texFont)
    {
        printf("texFont dimensions %dx%d\n", texFont->tex_width, texFont->tex_height);

        // Enable blending for texture alpha component
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Generate, bind, and upload font texture
        txfEstablishTexture(texFont, 0);

        // Set the GL texture's wrapping and stretching properties
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        fontSize[0] = (GLfloat)texFont->tex_width;
        fontSize[1] = (GLfloat)texFont->tex_height;

        updateShader(eventHandler);
     }
    else
        printf("error loading texFont\n");

}

void destroyFontTexture()
{
    txfUnloadFont(texFont);
}

void redraw(EventHandler& eventHandler)
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // All shaders use position geometry, so enable it here
    glEnableVertexAttribArray(vertexPositionIndex);

    // Draw a triangle with a colorful shader
    glUseProgram(triShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glVertexAttribPointer(vertexPositionIndex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Draw a texture atlas quad with a font texture shader
    glUseProgram(quadFontShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, quadFontVbo);
    glVertexAttribPointer(vertexPositionIndex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw text string quads with a text shader
    glEnableVertexAttribArray(vertexTexCoordIndex);
    glUseProgram(quadsTextShaderProgram);
    txfRenderString(texFont, "OpenGL", -64.0f * 2.5f, 0.0f);
    txfRenderString(texFont, "3D", -64.0f, -64.0f * 1.5f);
    glDisableVertexAttribArray(vertexTexCoordIndex);
   
    // Done with position geometry
    glDisableVertexAttribArray(vertexPositionIndex);

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
    initFontTexture(eventHandler);

    // Start the main loop
    void* mainLoopArg = &eventHandler;

#ifdef __EMSCRIPTEN__
    int fps = 0; // Use browser's requestAnimationFrame
    emscripten_set_main_loop_arg(mainLoop, mainLoopArg, fps, true);
#else
    while(true) 
        mainLoop(mainLoopArg);
#endif

    destroyFontTexture();

    return 0;
}