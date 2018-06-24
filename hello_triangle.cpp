//
// Minimal C++/SDL2/OpenGLES2 sample that Emscripten transpiles into Javascript/WebGL.
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
//     A colorful triangle.  Left mouse pans, wheel zooms in/out.
//
#include <exception>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#endif

SDL_Window* wnd = nullptr;
Uint32 windowID = 0;
int wndWidth = 640, wndHeight = 480;
bool mouseDown = false;

// Uniforms - frame invariant shader vars
GLint uniformPan, uniformZoom;
GLfloat pan[2] = {0.0f, 0.0f}, zoom = 1.0f;

// Vertex shader
const GLchar* vertexSource =
    "uniform vec2 pan;                             \n"
    "uniform float zoom;                           \n"
    "attribute vec4 position;                      \n"
    "varying vec3 color;                           \n"
    "void main()                                   \n"
    "{                                             \n"
    "    gl_Position = vec4(position.xyz, 1.0);    \n"
    "    gl_Position.xy += pan;                    \n"
    "    gl_Position.xy *= zoom;                   \n"
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

float clamp (float val, float lo, float hi)
{
    return std::max(lo, std::min(val, hi));
}

void handleEvents()
{
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_MOUSEMOTION: 
            {
                SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
                if (mouseDown)
                {
                    // Normalize cursor position to -2.0 to 2.0 in x and y
                    pan[0] = ((m->x / (float) wndWidth) - 0.5f) * 2.0f / zoom;
                    pan[1] = ((1.0f - (m->y / (float) wndHeight)) - 0.5f) * 2.0f / zoom;
                }
                break;
            }

            case SDL_MOUSEBUTTONDOWN: 
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                if (m->button == SDL_BUTTON_LEFT)
                {
                    mouseDown = true;

                    // Push a motion event to update display at current mouse
                    SDL_Event push_event;
                    push_event.type = SDL_MOUSEMOTION;
                    push_event.motion.x = m->x;
                    push_event.motion.y = m->y;
                    SDL_PushEvent(&push_event);                    
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: 
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                if (m->button == SDL_BUTTON_LEFT)
                    mouseDown = false;
                break;
            }

            case SDL_MOUSEWHEEL: 
            {
                SDL_MouseWheelEvent *m = (SDL_MouseWheelEvent*)&event;
                zoom += ((m->y < 0) ? -0.1f : 0.1f);
                zoom = clamp(zoom, 0.0f, 10.0f);
                break;
            }
        }
    }
}

void redraw()
{
    // Update uniforms
    glUniform2fv(uniformPan, 1, pan);
    glUniform1f(uniformZoom, zoom);

    // Clear the screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the vertex buffer
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Swap front/back framebuffers
    SDL_GL_SwapWindow(wnd);
}

void main_loop() 
{    
    handleEvents();
    redraw();
}

int main(int argc, char** argv)
{
#ifdef __EMSCRIPTEN__
    emscripten_get_canvas_element_size("#canvas", &wndWidth, &wndHeight);
#endif

    // Create SDL2 window with GL context
    wnd = SDL_CreateWindow("hello_triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            wndWidth, wndHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    windowID = SDL_GetWindowID(wnd);

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

    // Get uniform locations for updating later
	uniformPan = glGetUniformLocation(shaderProgram, "pan");
	uniformZoom = glGetUniformLocation(shaderProgram, "zoom");    

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
