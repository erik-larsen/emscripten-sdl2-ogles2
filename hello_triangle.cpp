//
// A minimal C++/SDL2/OpenGLES2 example that Emscripten can transpile into Javascript/WebGL.
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc hello_triangle.cpp -s USE_SDL=2 -s FULL_ES2=1 -o hello_triangle.js
//
// Run (open in browser):
//     index.html
//
// Result:
//     A colorful triangle. 
//
#include <exception>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#endif

// Vertex shader
const GLchar* vertexSource =
    "attribute vec4 position;                      \n"
    "varying vec3 color;                           \n"
    "void main()                                   \n"
    "{                                             \n"
    "    gl_Position = vec4(position.xyz, 1.0);    \n"
    "    color = gl_Position.xyz + vec3(0.5);      \n"
    "}                                             \n";

// Fragment/pixel shader
const GLchar* fragmentSource =
    "precision mediump float;                     \n"
    "varying vec3 color;                          \n"
    "void main()                                  \n"
    "{                                            \n"
    "    gl_FragColor = vec4 ( color, 1.0 );      \n"
    "}                                            \n";

SDL_Window* wnd;

void main_loop() 
{    
    // Check for events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) 
            std::terminate();
    }

    // Clear the screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the vertex buffer
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Swap front/back framebuffers
    SDL_GL_SwapWindow(wnd);
}

int main(int argc, char** argv)
{
    // Create SDL window with GL context
    wnd = SDL_CreateWindow("hello_triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GLContext glc = SDL_GL_CreateContext(wnd);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Create a vertex buffer object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = 
    {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Specify the layout of the shader vertex data (positions only, 3 floats)
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
#else
    while(true) 
        main_loop();
#endif

    return 0;
}
