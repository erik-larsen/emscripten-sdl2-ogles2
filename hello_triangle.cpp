//
// Minimal C++/SDL2/OpenGLES2 sample that Emscripten transpiles into Javascript/WebGL.
//
// Setup:
//     Install emscripten: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
//
// Build:
//     emcc hello_triangle.cpp -s USE_SDL=2 -s FULL_ES2=1 -o hello_triangle.js
//
// Run:
//     emrun hello_triangle.html
//     emrun hello_triangle_debug.html
//
// Result:
//     A colorful triangle.  Left mouse pans, mouse wheel zooms in/out.  Window is resizable.
//
#include <exception>
#include <algorithm>

#define GL_GLEXT_PROTOTYPES 1

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL.h>
#include <SDL_opengles2.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#endif

// Window
SDL_Window* window = nullptr;
Uint32 windowID = 0;
int windowWidth = 640, windowHeight = 480;

// Inputs
bool mouseDown = false, fingerDown = false, pinch = false;
float pinchDist = 0.0f;

// Shader vars
GLint shaderPan, shaderZoom, shaderAspect;
GLfloat pan[2] = {0.0f, 0.0f}, zoom = 1.0f, aspect = 1.0f;

// Vertex shader
const GLchar* vertexSource =
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

void updateShader()
{
    glUniform2fv(shaderPan, 1, pan);
    glUniform1f(shaderZoom, zoom); 
    glUniform1f(shaderAspect, aspect);
}

void resizeEvent(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    // Update viewport and aspect ratio
    glViewport(0, 0, windowWidth, windowHeight);
    aspect = windowWidth / (float)windowHeight; 
    updateShader();
}

void zoomEvent(bool wheelDown)
{                
    // Zoom by scaling up/down in 0.1 increments 
    zoom += (wheelDown ? -0.1f : 0.1f);
    zoom = clamp(zoom, 0.1f, 10.0f);
    updateShader();
}

void panEventMouse(int x, int y)
{ 
    // Make display follow cursor by normalizing cursor to range -2,2, scaled by inverse zoom 
    pan[0] = ((x / (float) windowWidth) - 0.5f) * 2.0f / zoom;
    pan[1] = ((1.0f - (y / (float) windowHeight)) - 0.5f) * 2.0f / zoom / aspect;
    updateShader();
}

void panEventFinger(float x, float y)
{ 
    pan[0] = (x - 0.5f) * 2.0f / zoom;
    pan[1] = (1.0f - y - 0.5f) * 2.0f / zoom / aspect;
    updateShader();
}

void handleEvents()
{
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                std::terminate();
                break;

            case SDL_WINDOWEVENT:
            {
                if (event.window.windowID == windowID
                    && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    resizeEvent(event.window.data1, event.window.data2);
                }
                break;
            }

            case SDL_MOUSEWHEEL: 
            {
                SDL_MouseWheelEvent *m = (SDL_MouseWheelEvent*)&event;
                bool wheelDown = m->y < 0;
                zoomEvent(wheelDown);
                break;
            }
            
            case SDL_MOUSEMOTION: 
            {
                SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
                if (mouseDown && !fingerDown)
                    panEventMouse(m->x, m->y);
                break;
            }

            case SDL_MOUSEBUTTONDOWN: 
            {
                SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
                if (m->button == SDL_BUTTON_LEFT && !fingerDown)
                {
                    mouseDown = true;

                    // Push a motion event to update display at current mouse position
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

            case SDL_FINGERMOTION:
                if (fingerDown)
                {
                    SDL_TouchFingerEvent *m = (SDL_TouchFingerEvent*)&event;
                    panEventFinger(m->x, m->y);
                }
                break;

            case SDL_FINGERDOWN:
                fingerDown = true;
                break;

            case SDL_FINGERUP:
                fingerDown = false;
                break;

            case SDL_MULTIGESTURE:
                SDL_MultiGestureEvent *m = (SDL_MultiGestureEvent*)&event;
                if (fabs(m->dDist) > 0.002f)
                {
                    pinch = true;
                    pinchDist = m->dDist;  // positive=open, negative=close
                    printf ("fingers=%d\n",m->numFingers);
                }
                break;
        }

        // Debugging
        printf ("event=%d mouse=%d finger=%d pinch=%d pinchDist=%f pan=%f,%f zoom=%f aspect=%f window=%dx%d\n", 
                 event.type, mouseDown, fingerDown, pinch, pinchDist, pan[0], pan[1], zoom, aspect, windowWidth, windowHeight);
    }
}

void redraw()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the vertex buffer
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Swap front/back framebuffers
    SDL_GL_SwapWindow(window);
}

void mainLoop() 
{    
    handleEvents();
    redraw();
}

int main(int argc, char** argv)
{
    // Create SDL window
    window = SDL_CreateWindow("hello_triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE| SDL_WINDOW_SHOWN);
    windowID = SDL_GetWindowID(window);

    // Create OpenGLES 2 context on window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GLContext glc = SDL_GL_CreateContext(window);

    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link vertex and fragment shader into shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Get shader variables and initalize them
	shaderPan = glGetUniformLocation(shaderProgram, "pan");
	shaderZoom = glGetUniformLocation(shaderProgram, "zoom");    
	shaderAspect = glGetUniformLocation(shaderProgram, "aspect");
    updateShader();

    // Create vertex buffer object and copy vertex data into it
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

    // Start the main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, true);
#else
    while(true) 
        mainLoop();
#endif

    return 0;
}
